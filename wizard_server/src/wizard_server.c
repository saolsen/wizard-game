// This is the dedicated server for wizard-game. It only targets linux (because it will use a lot of linux features) and
// has a client on port 40000 which is a UDP netcode.io server.
// It's probably gonna def have startup hooks and shit for auth since I want to launch these on demand.

// Goals for v1
// let clients emit chat messages, relay those messages to all other connected clients
// store a position for clients, they are gonna walk around.

// later
// emit events for shit that can be dtrace style traced
// save all state that happens to a sqlite database in such a way that it can be replayed (wow).
// ball harder

// There's like other shit that has to happen on start and end.
// on start we need the right creds and a list of users and stuff. (maybe)
// on teardown (if it's prod) we need to copy the sqlite log somewhere, prolly manta if we're on triton or s3 or whatever.

// honestly imma prolly try to get this on aws.
// digital ocean sounds really great too....

#include "netcode.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <inttypes.h>

static volatile int quit = 0;

void interrupt_handler( int signal )
{
    (void) signal;
    quit = 1;
}

// @HARDCODE: Shared private key between client and server.
static uint8_t private_key[NETCODE_KEY_BYTES] = { 0x60, 0x6a, 0xbe, 0x6e, 0xc9, 0x19, 0x10, 0xea, 
                                                  0x9a, 0x65, 0x62, 0xf6, 0x6f, 0x2b, 0x30, 0xe4, 
                                                  0x43, 0x71, 0xd6, 0x2c, 0xd1, 0x99, 0x27, 0x26,
                                                  0x6b, 0x3c, 0x60, 0xf4, 0xb7, 0x15, 0xab, 0xa1 };

int main(int argc, char **argv) {

    if (netcode_init() != NETCODE_OK)
    {
        printf( "error: failed to initialize netcode.io\n" );
        return 1;
    }

    netcode_log_level(NETCODE_LOG_LEVEL_INFO);

    double time = 0.0;
    // @Q: is 60fps the way to do it?
    double delta_time = 1.0 / 60.0;

    // @HARDCODE
    #define TEST_PROTOCOL_ID 0x1122334455667788

    // @HARDCODE
    char * server_address = "127.0.0.1:40000";

    struct netcode_server_t *server = netcode_server_create(server_address, TEST_PROTOCOL_ID, private_key, time);

    if (!server) {
        printf("error: failed to create server\n");
        return 1;
    }

    netcode_server_start(server, NETCODE_MAX_CLIENTS);

    signal(SIGINT, interrupt_handler);

    // Storage for packets.
    uint8_t packet_data[NETCODE_MAX_PACKET_SIZE];
    int i;
    for (i = 0; i < NETCODE_MAX_PACKET_SIZE; ++i)
        packet_data[i] = (uint8_t) i;

    while (!quit) {
        // @Q: What does this do?
        netcode_server_update(server, time);

        if (netcode_server_client_connected( server, 0 )) {
            // @Q: What packet are we sending?
            netcode_server_send_packet( server, 0, packet_data, NETCODE_MAX_PACKET_SIZE );
        }

        int client_index;
        for ( client_index = 0; client_index < NETCODE_MAX_CLIENTS; ++client_index ) {
            for (;;) {
                int packet_bytes;
                uint64_t packet_sequence;
                void * packet = netcode_server_receive_packet(server, client_index, &packet_bytes, &packet_sequence);
                if (!packet) { break; }
                (void) packet_sequence;

                printf("received a packet from client %i\n", client_index);

                // @Q: Not quite sure what this does but it explodes for custom packets.
                assert(packet_bytes == NETCODE_MAX_PACKET_SIZE);
                // @Q: This is checking that the whole packet maches whatever is stored in packet_data above.
                // I dunno why I would wish for that.... I think I just have to check the first n bytes or something?
                assert(memcmp(packet, packet_data, NETCODE_MAX_PACKET_SIZE) == 0);

                netcode_server_free_packet(server, packet);
            }
        }

        // @Q: What does this really do?
        netcode_sleep(delta_time);

        time += delta_time;
    }

    // @TODO: Better control flow and integration with however this is being run.
    if (quit) {
        printf("\nshutting down\n");
    }

    netcode_server_destroy(server);

    netcode_term();
    
    return 0;
}
