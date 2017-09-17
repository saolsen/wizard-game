// Do 1 more pass of this, just check on initialization and teardown state.
// I think this is just about good enough to write some basic test code againts.

// Still @TODO
// - Message based interface with reliable / non reliable.
// - Use reliable.io to track packet acks and handle them.
// - Handle splitted up big packets better.

#ifndef _wizard_network_h
#define _wizard_network_h

#include <malloc.h>
#include <inttypes.h>
#include <stdint.h>

#include "netcode.h"
#include "reliable.h"

#define LEN(e) (sizeof(e)/sizeof(e[0]))

// @TODO: Pull this out in some way that it's useable other places too.
// @OPTIMIZATION: Can use modulo and if power of 2 can use & (size-1) insetad of % size 
int enqueue(uint64_t *data, int capacity, int *head, int *tail, uint64_t e) {
    int next = *head+1;
    if (next >= capacity) {
        next = 0;
    }
    if (next == *tail) {
        return -1; // queue is full
    }
    data[*head] = e;
    *head = next;
    return 0;
}

// returns 0 if queue is empty or 1 if there's an element.
// makes it easier to use in a while loop.
// holy shit do I really need to figure out my return code life.
// @TODO
int dequeue(uint64_t *data, int capacity, int *head, int *tail, uint64_t *e) {
    if (*head == *tail) { // empty
        return 0;
    }
    int next = *tail + 1;
    if (next >= capacity) {
        next = 0;
    }
    *e = data[*tail];
    *tail = next;
    return 1;
}

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
    int connected;
    uint64_t client_id;

    void* current_packet;
    size_t current_packet_size;
    uint16_t current_packet_sequence;
    uint8_t *current_packet_data;
} NetworkClient;

int _client_process_packet_function(void* _context, int index, uint16_t sequence, uint8_t *packet_data, int packet_size);
void _client_transmit_packet_function(void *_context, int index, uint16_t sequence, uint8_t *packet_data, int packet_size);
void _client_state_change_function(void* _context, int old_state, int new_state);

char *my_netcode_client_state_name(int state)
{
    switch(state) {
    case NETCODE_CLIENT_STATE_CONNECT_TOKEN_EXPIRED:
        return "NETCODE_CLIENT_STATE_CONNECT_TOKEN_EXPIRED";
    case NETCODE_CLIENT_STATE_INVALID_CONNECT_TOKEN:
        return "NETCODE_CLIENT_STATE_INVALID_CONNECT_TOKEN";
    case NETCODE_CLIENT_STATE_CONNECTION_TIMED_OUT:
        return "NETCODE_CLIENT_STATE_CONNECTION_TIMED_OUT";
    case NETCODE_CLIENT_STATE_CONNECTION_RESPONSE_TIMED_OUT:
        return "NETCODE_CLIENT_STATE_CONNECTION_RESPONSE_TIMED_OUT";
    case NETCODE_CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT:
        return "NETCODE_CLIENT_STATE_CONNECTION_REQUEST_TIMED_OUT";
    case NETCODE_CLIENT_STATE_CONNECTION_DENIED:
        return "NETCODE_CLIENT_STATE_CONNECTION_DENIED";
    case NETCODE_CLIENT_STATE_DISCONNECTED:
        return "NETCODE_CLIENT_STATE_DISCONNECTED";
    case NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST:
        return "NETCODE_CLIENT_STATE_SENDING_CONNECTION_REQUEST";
    case NETCODE_CLIENT_STATE_SENDING_CONNECTION_RESPONSE:
        return "NETCODE_CLIENT_STATE_SENDING_CONNECTION_RESPONSE";
    case NETCODE_CLIENT_STATE_CONNECTED:
        return "NETCODE_CLIENT_STATE_CONNECTED";
    }
    return "UNKNOWN_STATE";
}

int client_connect(NetworkClient *client, double time, uint8_t *connect_token) {
    if (netcode_init() != NETCODE_OK) {
        printf( "error: failed to initialize netcode.io\n" );
        return 1;
    }
    reliable_init();
    client->reliable_endpoint = NULL;
    client->connected = 0;

    netcode_log_level(NETCODE_LOG_LEVEL_INFO);

    client->time = time;
    client->netcode_client = netcode_client_create("0.0.0.0", 0);
    if (!client->netcode_client) {
        printf("Error: Failed to create netcode client\n");
        return 1;
    }

    uint8_t default_connect_token[NETCODE_CONNECT_TOKEN_BYTES];
    if (!connect_token) {
        // Generate hardcoded test token for dev.
        NETCODE_CONST char *server_address = "127.0.0.1:40000";

        uint64_t client_id = 0;
        netcode_random_bytes( (uint8_t*) &client_id, 8 );
        printf("client id is %.16" PRIx64 "\n", client_id);
        client->client_id = client_id;

        if (netcode_generate_connect_token( 1, &server_address, CONNECT_TOKEN_EXPIRY, CONNECT_TOKEN_TIMEOUT, client_id, PROTOCOL_ID, 0, private_key, default_connect_token) != NETCODE_OK) {
            printf("error: failed to generate connect token\n");
            return 1;
        }
        connect_token = default_connect_token;   
    }

    netcode_client_state_change_callback(client->netcode_client, client, &_client_state_change_function);
    netcode_client_connect(client->netcode_client, default_connect_token);

    client->current_packet = NULL;
    client->current_packet_size = 0;
    client->current_packet_sequence = 0;
    client->current_packet_data = NULL;

    return 0;
}

void _client_state_change_function(void* _context, int old_state, int new_state)
{
    NetworkClient *client = (NetworkClient*)_context;

    printf("Client state changed from '%s' to '%s'\n",
        my_netcode_client_state_name(old_state),
        my_netcode_client_state_name(new_state));
    
        if (new_state == NETCODE_CLIENT_STATE_CONNECTED) {
            client->connected = 1;
            if (client->reliable_endpoint) {
                reliable_endpoint_destroy(client->reliable_endpoint);
            }

            // Set up reliable
            struct reliable_config_t reliable_config;
            reliable_default_config(&reliable_config);
            reliable_config.fragment_above = 500; // @Q: What size do we want for this?
            reliable_config.index = 0;
            reliable_config.process_packet_function = &_client_process_packet_function;
            reliable_config.transmit_packet_function = &_client_transmit_packet_function;
            reliable_config.context = client;
            client->reliable_endpoint = reliable_endpoint_create(&reliable_config, client->time);
        } else {
            client->connected = 0;
            if (client->reliable_endpoint) {
                reliable_endpoint_destroy(client->reliable_endpoint);
                client->reliable_endpoint = NULL;
            }
        }
}

void client_update(NetworkClient *client, double time) {
    client->time = time;
    netcode_client_update(client->netcode_client, time);
    if (client->connected) {
        reliable_endpoint_update(client->reliable_endpoint, time);
    }
}

int _client_process_packet_function(void* _context, int index, uint16_t sequence, uint8_t *packet_data, int packet_size)
{
    NetworkClient *client = (NetworkClient*)_context;
    client->current_packet_size = (size_t)packet_size;
    client->current_packet_sequence = sequence;
    client->current_packet_data = packet_data;
    return 0;
}

uint8_t *client_packet_receive(NetworkClient *client, size_t *packet_size, uint64_t *packet_sequence)
{
    if (!client->connected) { return NULL; }

    int packet_bytes;
    void *packet = netcode_client_receive_packet(client->netcode_client, &packet_bytes, packet_sequence);
    if (!packet) { return NULL;}
    
    client->current_packet = packet;
    client->current_packet_size = 0;
    client->current_packet_sequence = 0;
    client->current_packet_data = NULL;
    
    reliable_endpoint_receive_packet(client->reliable_endpoint, packet, packet_bytes);
    
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
void client_packet_send(NetworkClient *client, uint8_t *data, size_t size)
{
    reliable_endpoint_send_packet(client->reliable_endpoint, data, (int)size);
}

void client_send_packets(NetworkClient *client)
{
    // @TODO: Will send queued up packets and resend any reliable messages that need it.
}

void client_destroy(NetworkClient *client)
{
    netcode_client_disconnect(client->netcode_client);
    if (client->reliable_endpoint) {
        reliable_endpoint_destroy(client->reliable_endpoint);
        client->reliable_endpoint = NULL;
    }
    reliable_term();
    netcode_client_destroy(client->netcode_client);
    client->netcode_client = NULL;
    netcode_term();
}

#define MAX_CONNECTED_CLIENTS 32

typedef struct {
    double time;
    struct netcode_server_t *netcode_server;
    
    // State stored per connected client. Indexed by server slot.
    uint64_t client_ids[MAX_CONNECTED_CLIENTS];
    struct reliable_endpoint_t* reliable_endpoints[MAX_CONNECTED_CLIENTS];

    void* current_packet;
    size_t current_packet_size;
    uint16_t current_packet_sequence;
    uint8_t *current_packet_data;

    uint64_t client_connects[MAX_CONNECTED_CLIENTS];
    int client_connects_head;
    int client_connects_tail;

    uint64_t client_disconnects[MAX_CONNECTED_CLIENTS];
    int client_disconnects_head;
    int client_disconnects_tail;
} NetworkServer;

int _server_process_packet_function(void *_context, int index, uint16_t sequence, uint8_t *packet_data, int packet_size);
void _server_transmit_packet_function(void *_context, int index, uint16_t sequence, uint8_t *packet_data, int packet_size);
void _server_connect_disconnect_function(void *_context, int client_index, int connected);

// @TODO: There's a lot more to this for having real clients.
// There's a private key and protocol id and a bunch of other stuff.
// Once I get walk around and press button to wave, I'm gonna do real auth stuff and matchmaking.
int server_run(NetworkServer *server, double time)
{
    server->time = time;

    if (netcode_init() != NETCODE_OK) {
        printf("Error: failed to initialize netcode.io\n");
        return 1;
    }
    
    netcode_log_level(NETCODE_LOG_LEVEL_INFO);

    // @HARDCODE
    char *server_address = "127.0.0.1:40000";
    server->netcode_server = netcode_server_create(server_address, PROTOCOL_ID, private_key, time);
    if (!server) {
        printf("Error: failed to create server\n");
        return 1;
    }

    for (int i=0; i<MAX_CONNECTED_CLIENTS; i++) {
        server->client_ids[i] = 0;
        server->reliable_endpoints[i] = NULL;
    }

    reliable_init();

    netcode_server_connect_disconnect_callback(server->netcode_server, server, &_server_connect_disconnect_function);
    netcode_server_start(server->netcode_server, MAX_CONNECTED_CLIENTS);

    server->current_packet = NULL;
    server->current_packet_size = 0;
    server->current_packet_sequence = 0;
    server->current_packet_data = NULL;

    server->client_connects_head = 0;
    server->client_connects_tail = 0;
    server->client_disconnects_head = 0;
    server->client_disconnects_tail = 0;
    
    return 0;
}

void _server_connect_disconnect_function(void *_context, int client_index, int connected)
{
    NetworkServer *server = (NetworkServer*)_context;

    if (connected) {
        uint64_t client_id = netcode_server_client_id(server->netcode_server, client_index);
        server->client_ids[client_index] = client_id;
        enqueue(server->client_connects, LEN(server->client_connects), &server->client_connects_head, &server->client_connects_tail, client_id);
        
        struct reliable_config_t reliable_config;
        reliable_default_config(&reliable_config);
        reliable_config.fragment_above = 500;
        reliable_config.index = 0;
        reliable_config.process_packet_function = &_server_process_packet_function;
        reliable_config.transmit_packet_function = &_server_transmit_packet_function;
        reliable_config.context = server;
        reliable_config.index = client_index;
        server->reliable_endpoints[client_index] = reliable_endpoint_create(&reliable_config, server->time);
    } else {
        uint64_t client_id = server->client_ids[client_index];
        server->client_ids[client_index] = 0;

        enqueue(server->client_disconnects, LEN(server->client_disconnects), &server->client_disconnects_head, &server->client_disconnects_tail, client_id);
        
        reliable_endpoint_destroy(server->reliable_endpoints[client_index]);
        server->reliable_endpoints[client_index] = NULL;
    }

}

// Called at the beginning of the frame with the current time.
void server_update(NetworkServer *server, double time) {
    server->time = time;
    netcode_server_update(server->netcode_server, time);
    for (int client_index = 0; client_index < MAX_CONNECTED_CLIENTS; client_index++) {
        if (server->client_ids[client_index]) {
            reliable_endpoint_update(server->reliable_endpoints[client_index], time);
        }
    }
}

int server_client_connects(NetworkServer *server, int *client_index, uint64_t *client_id)
{
    return 0;
}

int server_client_disconnects(NetworkServer *server, int *client_index, uint64_t *client_id)
{
    return 0;
}

int server_clients(NetworkServer *server, int *client_index, uint64_t *client_id)
{
    return 0;
}

int _server_process_packet_function(void *_context, int index, uint16_t sequence, uint8_t *packet_data, int packet_size)
{
    NetworkServer *server = (NetworkServer*)_context;
    server->current_packet_size = (size_t)packet_size;
    server->current_packet_sequence = sequence;
    server->current_packet_data = packet_data;
    return 0;
}

// @BUG: Playing real loose with memory here, gotta figure out how to actually manage and free the different parts.
uint8_t *server_packet_receive(NetworkServer *server, uint64_t client_id, size_t *packet_size, uint64_t *packet_sequence)
{
    for (int client_index = 0; client_index < MAX_CONNECTED_CLIENTS; client_index++) {
        if (server->client_ids[client_index] == client_id) {
            int packet_bytes;
            void *packet = netcode_server_receive_packet(server->netcode_server, client_index, &packet_bytes, packet_sequence);
            if (!packet) { return NULL; }

            server->current_packet = packet;
            server->current_packet_size = 0;
            server->current_packet_sequence = 0;
            server->current_packet_data = NULL;
    
            reliable_endpoint_receive_packet(server->reliable_endpoints[client_index], packet, packet_bytes);

            *packet_size = server->current_packet_size;
            *packet_sequence = server->current_packet_sequence;
            return server->current_packet_data;
        }
    }

    return NULL;
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
    NetworkServer *server = (NetworkServer*)_context;
    netcode_server_send_packet(server->netcode_server, index, packet_data, packet_size);
}

// @TODO: change to message based, message_send_reliable, message_send_unreliable, maybe also some broadcasting
// stuff that makes it easy to send to all connected clients.
// also like buffer broadcast that will resend all past messages to all newly connected clients.

void server_packet_send(NetworkServer *server, uint64_t client_id, uint8_t *data, size_t size)
{
    for (int client_index = 0; client_index < MAX_CONNECTED_CLIENTS; client_index++) {
        if (server->client_ids[client_index] == client_id) {
            reliable_endpoint_send_packet(server->reliable_endpoints[client_index], data, (int)size);
            break;
        }
    }
}

void server_destroy(NetworkServer *server)
{
    for (int client_index = 0; client_index < MAX_CONNECTED_CLIENTS; client_index++) {
        if (server->reliable_endpoints[client_index]) {
            reliable_endpoint_destroy(server->reliable_endpoints[client_index]);
            server->reliable_endpoints[client_index] = NULL;
        }
    }
    reliable_term();
    netcode_server_destroy(server->netcode_server);
    server->netcode_server = NULL;
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