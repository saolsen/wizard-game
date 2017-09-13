#ifndef _wizard_network_h
#define _wizard_network_h

#include <malloc.h>
#include <inttypes.h>
#include <stdint.h>

#include "netcode.h"
#include "reliable.h"

static uint8_t private_key[NETCODE_KEY_BYTES] = { 0x60, 0x6a, 0xbe, 0x6e, 0xc9, 0x19, 0x10, 0xea, 
                                                  0x9a, 0x65, 0x62, 0xf6, 0x6f, 0x2b, 0x30, 0xe4, 
                                                  0x43, 0x71, 0xd6, 0x2c, 0xd1, 0x99, 0x27, 0x26,
                                                  0x6b, 0x3c, 0x60, 0xf4, 0xb7, 0x15, 0xab, 0xa1 };

#define CONNECT_TOKEN_EXPIRY 30
#define CONNECT_TOKEN_TIMEOUT 5
#define PROTOCOL_ID 0x1122334455667788

typedef struct {
    double time;
    struct netcode_client_t *netcode_client;
    struct reliable_endpoint_t *reliable_endpoint;

    void* current_packet;
    uint32_t current_packet_size;
    uint16_t current_packet_sequence;
    uint8_t *current_packet_data;
} NetworkClient;

int _client_process_packet_function(void* _context, int index, uint16_t sequence, uint8_t *packet_data, int packet_size);
void _client_transmit_packet_function(void *_context, int index, uint16_t sequence, uint8_t *packet_data, int packet_size);

int client_connect(NetworkClient *client, double time, uint8_t *connect_token) {
    if (netcode_init() != NETCODE_OK) {
        printf( "error: failed to initialize netcode.io\n" );
        return 1;
    }
    reliable_init();

    netcode_log_level(NETCODE_LOG_LEVEL_INFO);

    client->time = time;
    client->netcode_client = netcode_client_create("0.0.0.0", 0);
    if (!client->netcode_client) {
        printf("Error: Failed to create netcode client\n");
        return 1;
    }

    if (!connect_token) {
        // Generate hardcoded test token for dev.
        NETCODE_CONST char *server_address = "127.0.0.1:40000";

        uint64_t client_id = 0;
        netcode_random_bytes( (uint8_t*) &client_id, 8 );
        printf("client id is %.16" PRIx64 "\n", client_id);

        uint8_t default_connect_token[NETCODE_CONNECT_TOKEN_BYTES];

        if (netcode_generate_connect_token( 1, &server_address, CONNECT_TOKEN_EXPIRY, CONNECT_TOKEN_TIMEOUT, client_id, PROTOCOL_ID, 0, private_key, default_connect_token) != NETCODE_OK) {
            printf("error: failed to generate connect token\n");
            return 1;
        }

        netcode_client_connect(client->netcode_client, default_connect_token);
    } else {
        netcode_client_connect(client->netcode_client, connect_token);
    }

    // @TODO: Check result

    struct reliable_config_t reliable_config;
    reliable_default_config(&reliable_config);
    reliable_config.fragment_above = 500; // @Q: What size do we use for this?
    reliable_config.index = 0; // @Q: What is index.
    reliable_config.process_packet_function = &_client_process_packet_function;
    reliable_config.transmit_packet_function = &_client_transmit_packet_function;
    reliable_config.context = client;
    client->reliable_endpoint = reliable_endpoint_create(&reliable_config, time);

    return 0;
}

void client_update(NetworkClient *client, double time) {
    client->time = time;
    netcode_client_update(client->netcode_client, time);
    reliable_endpoint_update(client->reliable_endpoint, time);
}

int _client_process_packet_function(void* _context, int index, uint16_t sequence, uint8_t *packet_data, int packet_size)
{
    NetworkClient *client = (NetworkClient*)_context;
    client->current_packet_size = packet_size;
    client->current_packet_sequence = sequence;
    client->current_packet_data = packet_data;
    return 0;
}

uint8_t *client_packet_receive(NetworkClient *client, uint32_t *packet_size, uint64_t *packet_sequence)
{
    uint32_t packet_bytes;
    void *packet = netcode_client_receive_packet(client->netcode_client, &packet_bytes, packet_sequence);
    if (!packet) {
        return NULL;
    }
    
    // This unwraps the reliable stuff from the packet and calls our callback _client_process_packet_function
    // which saves the packet to some temp vars in the client.
    client->current_packet = packet;
    reliable_endpoint_receive_packet(client->reliable_endpoint, packet, packet_bytes);
    // @TODO: Need to check if there's anything saved. If it's a partial these won't be set.
    *packet_size = client->current_packet_size;
    *packet_sequence = client->current_packet_sequence;
    return client->current_packet_data;
}

void client_packet_free(NetworkClient *client)
{
    netcode_client_free_packet(client->netcode_client, client->current_packet);
}

void _client_transmit_packet_function(void *_context, int index, uint16_t sequence, uint8_t *packet_data, int packet_size)
{
    NetworkClient *client = (NetworkClient*)_context;
    netcode_client_send_packet(client->netcode_client, packet_data, packet_size);
}

// @TODO: this will be message_send_reliable and message_send_unreliable
// @TODO: This will also only queue up sends.
void client_packet_send(NetworkClient *client, uint8_t *data, uint32_t size)
{
    reliable_endpoint_send_packet(client->reliable_endpoint, data, size);
}

void client_send_packets(NetworkClient *client)
{
    // @TODO: Will send queued up packets and resend any reliable messages that need it.
}

void client_destroy(NetworkClient *client)
{
    netcode_client_destroy(client->netcode_client);
    reliable_term();
    netcode_term();
}

#define MAX_CONNECTED_CLIENTS 32

typedef struct {
    double time;
    struct netcode_server_t *netcode_server;
    struct reliable_endpoint_t* reliable_endpoints[MAX_CONNECTED_CLIENTS];

    void* current_packet;
    uint32_t current_packet_size;
    uint16_t current_packet_sequence;
    uint8_t *current_packet_data;
} NetworkServer;

int _server_process_packet_function(void *_context, int index, uint16_t sequence, uint8_t *packet_data, int packet_size);
void _server_transmit_packet_function(void *_context, int index, uint16_t sequence, uint8_t *packet_data, int packet_size);

// @TODO: There's a lot more to this for having real clients.
// There's a private key and protocol id and a bunch of other stuff.
// Once I get walk around and press button to wave, I'm gonna do real auth stuff and matchmaking.
int server_run(NetworkServer *server, double time)
{
    if (netcode_init() != NETCODE_OK) {
        printf("Error: failed to initialize netcode.io\n");
        return 1;
    }
    netcode_log_level(NETCODE_LOG_LEVEL_INFO);

    for (int i=0; i<MAX_CONNECTED_CLIENTS; i++) {
        server->reliable_endpoints[i] = NULL;
    }

    // @HARDCODE
    char *server_address = "127.0.0.1:40000";
    server->netcode_server = netcode_server_create(server_address, PROTOCOL_ID, private_key, time);
    if (!server) {
        printf("Error: failed to create server\n");
        return 1;
    }

    reliable_init();
    // @TODO: figure out how you tell when there's a new client connected.
    netcode_server_start(server->netcode_server, MAX_CONNECTED_CLIENTS);
    return 0;
}

void server_update(NetworkServer *server, double time) {
    server->time = time;
    netcode_server_update(server->netcode_server, time);
    // @TODO: Update reliable endpoints too.
    //reliable_endpoint_update(client->reliable_endpoint, time);
}

struct wrapped_server {
    NetworkServer *server;
    int client_index;
};

int _server_process_packet_function(void *_context, int index, uint16_t sequence, uint8_t *packet_data, int packet_size)
{
    struct wrapped_server *wrap = (struct wrapped_server*)_context;
    wrap->server->current_packet_size = packet_size;
    wrap->server->current_packet_sequence = sequence;
    wrap->server->current_packet_data = packet_data;
    return 0;
}

// @BUG: Playing real loose with memory here, gotta figure out how to actually manage and free the different parts.
uint8_t *server_packet_receive(NetworkServer *server, int client_index, uint32_t *packet_size, uint64_t *packet_sequence)
{
    // @TODO: This is really janky. Think through the lifecycle of client connections and create the state
    // correctly. (like probably not here, only clearing if there's a request ask with disconnected client is bad)
    // figure out how to really learn a new client is connected through netcode.
    if (!netcode_server_client_connected(server->netcode_server, client_index)) {
        if (server->reliable_endpoints[client_index] != NULL) {
            //@TODO: free(server->reliable_endpoints[client_index].context);
            reliable_endpoint_destroy(server->reliable_endpoints[client_index]);
            server->reliable_endpoints[client_index] = NULL;
        }
        return NULL;
    }

    if (!server->reliable_endpoints[client_index]) {
        // Create reliable endpoint. @TODO: again, this is really not the place for this.
        struct reliable_config_t reliable_config;
        reliable_default_config(&reliable_config);
        reliable_config.fragment_above = 500;
        reliable_config.index = 0;
        reliable_config.process_packet_function = &_server_process_packet_function;
        reliable_config.transmit_packet_function = &_server_transmit_packet_function;
        struct wrapped_server *wrap = malloc(sizeof(*wrap));
        wrap->server = server;
        wrap->client_index = client_index;
        reliable_config.context = wrap;
        server->reliable_endpoints[client_index] = reliable_endpoint_create(&reliable_config, server->time);
    }

    uint32_t packet_bytes;
    void *packet = netcode_server_receive_packet(server->netcode_server, client_index, &packet_bytes, packet_sequence);
    if (!packet) { return NULL; }
    server->current_packet = packet;
    reliable_endpoint_receive_packet(server->reliable_endpoints[client_index], packet, packet_bytes);
    // @TODO: Need to check if there's anything saved. If it's a partial these won't be set.
    *packet_size = server->current_packet_size;
    *packet_sequence = server->current_packet_sequence;
    return server->current_packet_data;
}

void server_packet_free(NetworkServer *server)
{
    if (server->current_packet) {
        netcode_server_free_packet(server->netcode_server, server->current_packet);
        server->current_packet = NULL;
    }
}

void _server_transmit_packet_function(void *_context, int index, uint16_t sequence, uint8_t *packet_data, int packet_size)
{
    struct wrapped_server *wrap = (struct wrapped_server*)_context;
    netcode_server_send_packet(wrap->server->netcode_server, wrap->client_index, packet_data, packet_size);
}

// @TODO: change to message based, message_send_reliable, message_send_unreliable, maybe also some broadcasting
// stuff that makes it easy to send to all connected clients.
// also like buffer broadcast that will resend all past messages to all newly connected clients.
void server_packet_send(NetworkServer *server, int client_index, uint8_t *data, uint32_t size)
{
    // @COPYPASTA: Same logic for creating reliable endpoint on the fly as above.
    // Don't wanna do it like this.
    if (!netcode_server_client_connected(server->netcode_server, client_index)) {
        if (server->reliable_endpoints[client_index]) {
            //@TODO: free(server->reliable_endpoints[client_index].context);
            reliable_endpoint_destroy(server->reliable_endpoints[client_index]);
            server->reliable_endpoints[client_index] = NULL;
        }
        return;
    }

    // @COPYPASTA
    if (!server->reliable_endpoints[client_index]) {
        // Create reliable endpoint. @TODO: again, this is really not the place for this.
        struct reliable_config_t reliable_config;
        reliable_default_config(&reliable_config);
        reliable_config.fragment_above = 500;
        reliable_config.index = 0;
        reliable_config.process_packet_function = &_server_process_packet_function;
        reliable_config.transmit_packet_function = &_server_transmit_packet_function;
        struct wrapped_server *wrap = malloc(sizeof(*wrap));
        wrap->server = server;
        wrap->client_index = client_index;
        reliable_config.context = wrap;
        server->reliable_endpoints[client_index] = reliable_endpoint_create(&reliable_config, server->time);
    }

    reliable_endpoint_send_packet(server->reliable_endpoints[client_index], data, size);
}

void server_destroy(NetworkServer *server)
{
    netcode_server_destroy(server->netcode_server);
    reliable_term();
    netcode_term();
}
#endif

#ifdef WIZARD_TESTING
// @TODO: What is the best way to really test this stuff.
// I think I need to use the built in mocking stuff netcode.io has and like really test the shit out of
// all my actuial use cases.

// Lets just start by recording some things to test.
// Reliable messages get re-sent.
// Big messages get split up, dropped parts get resent and once they all come through it works.
// Broadcast stuff.
// Buffered broadcast stuff.
// o boy a lot of shit...

// I should be able to reuse this for later multiplayer games tho, that's pretty dope.

TEST test_network_thingy(void) {
    ASSERT(true);
    PASS();
}

SUITE(wizard_network_tests) {
    RUN_TEST(test_network_thingy);
}
#endif