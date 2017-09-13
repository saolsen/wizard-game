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
#include "reliable.h"
#include "mpack.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <inttypes.h>

#include "wizard_network.h"

static volatile int quit = 0;

void interrupt_handler( int signal )
{
    (void) signal;
    quit = 1;
}

int main(int argc, char **argv)
{
    NetworkServer server = (NetworkServer){0};

    // @Q: is 60fps the way to do it?
    double time = 0.0;
    double delta_time = 1.0 / 60.0;

    // @TODO: Like a lot of work on auth and shit.
    server_run(&server, time);

    signal(SIGTERM, interrupt_handler);

    // Storage for packets.
    uint8_t packet_data[NETCODE_MAX_PACKET_SIZE];
    int i;
    for (i = 0; i < NETCODE_MAX_PACKET_SIZE; ++i)
        packet_data[i] = (uint8_t) i;

    while (!quit) {
        server_update(&server, time);

        // @TODO: API for this or good way to write code like this.
        if (netcode_server_client_connected(server.netcode_server, 0)) {
            server_packet_send(&server, 0, packet_data, NETCODE_MAX_PACKET_SIZE);
        }

        int client_index;
        for (client_index = 0; client_index < MAX_CONNECTED_CLIENTS; ++client_index) {
            for (;;) {
                int packet_size;
                uint64_t packet_sequence;
                void *packet = server_packet_receive(&server, client_index, &packet_size, &packet_sequence);
                if (!packet) { break; }

                if (packet_size != NETCODE_MAX_PACKET_SIZE) {
                    printf("received a packet from client %i\n", client_index);
                    
                    mpack_reader_t reader;
                    mpack_reader_init_data(&reader, packet, packet_size);
        
                    char key1[20];
                    bool b;
                    char key2[20];
                    uint32_t i;

                    mpack_expect_map_match(&reader, 2);
                    mpack_expect_cstr(&reader, key1, sizeof(key1));
                    b = mpack_expect_bool(&reader);
                    mpack_expect_cstr(&reader, key2, sizeof(key2));
                    i = mpack_expect_uint(&reader);
                    mpack_done_map(&reader);

                    mpack_reader_destroy(&reader);

                    fprintf(stderr, "{%s: %s, %s: %u}\n", key1, (b ? "true" : "false"), key2, i);
                }

                server_packet_free(&server);
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

    server_destroy(&server);
    return 0;
}
