#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <inttypes.h>
#include <malloc.h>

#include "wizard_network.h"
#include "wizard_message.h"

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

        // server_clients_connected_this_tick(); array of indexes, and a length
        // server_clients_disconnected_this_tick(); array of indexes and a length
        // server_clients_connected(); array of indexes and a length

        int client_index;
        for (client_index = 0; client_index < MAX_CONNECTED_CLIENTS; ++client_index) {
            for (;;) {
                int packet_size;
                uint64_t packet_sequence;
                void *packet = server_packet_receive(&server, client_index, &packet_size, &packet_sequence);
                if (!packet) { break; }

                if (packet_size != NETCODE_MAX_PACKET_SIZE) {
                    //printf("received a packet from client %i\n", client_index);
                    
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

                    //fprintf(stderr, "{%s: %s, %s: %u}\n", key1, (b ? "true" : "false"), key2, i);
                }

                server_packet_free(&server);
            }
        }

        // give me a list of new clients who connected this frame
        // give me a list of clients that disconnected this frame
        // handle all that.

        // @Q: What does this really do?
        fflush(stdout);
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
