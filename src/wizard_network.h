// Firstly this is just a copy over. 1 packet per message.
// Then I can do reliable stuff.
// Then I can do a message oriented interface where they get packed up together.

#ifndef _wizard_network_h
#define _wizard_network_h

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
} Client;

void _transmit_packet_function(void *_context, int index, uint16_t sequence, uint8_t *packet_data, int packet_size);
int _process_packet_function(void* _context, int index, uint16_t sequence, uint8_t *packet_data, int packet_size);

int client_connect(Client *client, double time, uint8_t *connect_token) {
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
    reliable_config.transmit_packet_function = &_transmit_packet_function;
    reliable_config.process_packet_function = &_process_packet_function;
    reliable_config.context = client;
    client->reliable_endpoint = reliable_endpoint_create(&reliable_config, time);

    return 0;
}

void client_update(Client *client, double time) {
    netcode_client_update(client->netcode_client, time);
    reliable_endpoint_update(client->reliable_endpoint, time);
}

// send

int _process_packet_function(void* _context, int index, uint16_t sequence, uint8_t *packet_data, int packet_size)
{
    Client *client = (Client*)_context;
    client->current_packet_size = packet_size;
    client->current_packet_sequence = sequence;
    client->current_packet_data = packet_data;
    return 0;
}

uint8_t *client_packet_receive(Client *client, uint32_t *packet_size, uint64_t *packet_sequence)
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

void client_packet_free(Client *client)
{
    netcode_client_free_packet(client->netcode_client, client->current_packet);
}

void _transmit_packet_function(void *_context, int index, uint16_t sequence, uint8_t *packet_data, int packet_size)
{
    Client *client = (Client*)_context;
    netcode_client_send_packet(client->netcode_client, packet_data, packet_size);
}

// @TODO: this will be message_send_reliable and message_send_unreliable
// @TODO: This will also only queue up sends.
void client_packet_send(Client *client, uint8_t *data, uint32_t size)
{
    reliable_endpoint_send_packet(client->reliable_endpoint, data, size);
}

void client_send_packets(Client *client)
{
    // @TODO: Will send queued up packets and resend any reliable messages that need it.
}

void client_destroy(Client *client)
{
    netcode_client_destroy(client->netcode_client);
    reliable_term();
    netcode_term();
}


//Init connection.
//Teardown connection.
//Status
//push reliable.
//push unreliable.
//update
//get_packet
//free_packet
//send_packets

// Get and Free lets us iterate packets and do a minimal amount of copying of the data.

// For the client this is fine, what about for the server where there's one netcode
// server but a reliable queue per client. I guess I do a connection per client?
// That means the netcode stuff has to be not managed here...

#endif