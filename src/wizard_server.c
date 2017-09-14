// stuff for wizard.h
// push
// pop
// allocate_freelist
// 

// You need like 2 fucking things to program.
// a stack
// a ring buffer
// stack is easy, just ++ --
// ring buffer is a bit more complicated
// array, put_index, get_index, some way to know if it's full...

// sometimes ya need a map too and occasionally all those things have to grow.

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

    typedef struct {
        uint64_t connected_clients[MAX_CONNECTED_CLIENTS];
        int num_connected_clients;
    } ServerGameState;

    ServerGameState _gamestate = {
        .connected_clients = {0},
        .num_connected_clients = 0,
    };

    ServerGameState *gamestate = &_gamestate;

    while (!quit) {
        server_update(&server, time);

        uint64_t client_id;
        while(dequeue(server.client_connects, LEN(server.client_connects), &server.client_connects_head, &server.client_connects_tail, &client_id)) {
            printf("[server] Client '%.16" PRIx64 "' has connected\n", client_id);
            gamestate->connected_clients[gamestate->num_connected_clients++] = client_id;
        }
        
        while(dequeue(server.client_disconnects, LEN(server.client_disconnects), &server.client_disconnects_head, &server.client_disconnects_tail, &client_id)) {
            printf("[server] Client '%.16" PRIx64 "' has disconnected\n", client_id);

            uint64_t last_id = 0;
            for (int i = gamestate->num_connected_clients - 1; i <= 0; i--) {
                uint64_t id = gamestate->connected_clients[i];
                gamestate->connected_clients[i] = last_id;
                last_id = id;
                if (id == client_id) {
                    break;
                }
            }
            gamestate->num_connected_clients--;
        }

        for (int i=0; i < gamestate->num_connected_clients; i++) {
            uint64_t client_id = gamestate->connected_clients[i];

            server_packet_send(&server, client_id, packet_data, NETCODE_MAX_PACKET_SIZE);

            for (;;) {
                int packet_size;
                uint64_t packet_sequence;
                void *packet = server_packet_receive(&server, client_id, &packet_size, &packet_sequence);
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
