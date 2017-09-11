// connection status.
// - log                       button

// server relays connections and button presses.
// This means all button presses have to be reliably sent.
// need to look into realiable.io.

#include "netcode.h"
#include "reliable.h"
#include "mpack.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define CONNECT_TOKEN_EXPIRY 30
#define CONNECT_TOKEN_TIMEOUT 5
#define PROTOCOL_ID 0x1122334455667788

// @TODO: Share this with server and client for test.
static uint8_t private_key[NETCODE_KEY_BYTES] = { 0x60, 0x6a, 0xbe, 0x6e, 0xc9, 0x19, 0x10, 0xea, 
                                                  0x9a, 0x65, 0x62, 0xf6, 0x6f, 0x2b, 0x30, 0xe4, 
                                                  0x43, 0x71, 0xd6, 0x2c, 0xd1, 0x99, 0x27, 0x26,
                                                  0x6b, 0x3c, 0x60, 0xf4, 0xb7, 0x15, 0xab, 0xa1 };

// These are set in the reliable config.
// I think when you send "RELIABLE SEND PACKET"
// it uses this function, I'm not sure when the process function gets called tho.
void transmit_packet_function(void* _context, int index, uint16_t sequence, uint8_t *packet_data, int packet_bytes)
{

}

void process_packet_function(void* _context, int index, uint16_6 sequence, uint8_t packet_data, int packet_bytes)
{

}

int main(int argc, char**argv ) {
    // raylib
    int screen_width = 1024;
    int screen_height = 768;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screen_width, screen_height, "Wizard Game");
    SetTargetFPS(60);

    if (netcode_init() != NETCODE_OK) {
        printf( "error: failed to initialize netcode.io\n" );
        return 1;
    }

    netcode_log_level( NETCODE_LOG_LEVEL_INFO );

    double time = 0.0;
    double delta_time = 1.0 / 60.0;

    printf( "[client]\n" );

    struct netcode_client_t *client = netcode_client_create("0.0.0.0", time);

    if (!client) {
        printf("error: failed to create client\n");
        return 1;
    }

    NETCODE_CONST char *server_address = (argc != 2) ? "127.0.0.1:40000" : argv[1];

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );
    printf("client id is %.16" PRIx64 "\n", client_id);

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    if (netcode_generate_connect_token( 1, &server_address, CONNECT_TOKEN_EXPIRY, CONNECT_TOKEN_TIMEOUT, client_id, PROTOCOL_ID, 0, private_key, connect_token) != NETCODE_OK) {
        printf("error: failed to generate connect token\n");
        return 1;
    }

    netcode_client_connect(client, connect_token);

    struct reliable_config_t reliable_config;
    reliable_default_config(&reliable_config);
    // @Q: What is this? I think this is the max size for a packet
    //     before it gets fragmented.
    //     I'm not sure what the default it.
    reliable_config.fragment_above = 500;
    // @TODO: Set the sender config name, I think for debugging.
    // @Q: What is the context.
    reliable_config.index = 0;
    reliable_config.transmit_packet_function = NULL;
    reliable_config.process_packet_function = NULL;

    // @TODO: I need to set this because this is all the transmit packet
    // and process packet functions have to get state. I probably
    // need to pass any of the netcode.io stuff in here.
    reliable_config.context = NULL; 
    reliable_config.transmit_packet_function = &transmit_packet_function;
    reliable_config.process_packet_function = &process_packet_function;

    reliable_endpoint_t reliable_endpoint = reliable_endpoint_create(&reliable_config, time);

    // then I think we use the reliable endpoint by hitting it with
    // reliable_endpoint_send_packet
    // and
    // reliable_endpoint_update
    // now I don't know what update does but I think that it resends
    // stuff that hasn't been acked. I dunno how it gets the packets tho.
    // @TODO: Figure it out.
    

    uint8_t packet_data[NETCODE_MAX_PACKET_SIZE];
    int i;
    for (i = 0; i < NETCODE_MAX_PACKET_SIZE; ++i)
        packet_data[i] = (uint8_t) i;

    while (!WindowShouldClose()) {
        // I guess I call reliable_endpoint_update here.
        netcode_client_update(client, time);

        if (netcode_client_state(client) == NETCODE_CLIENT_STATE_CONNECTED) {
            char *data;
            size_t size;
            mpack_writer_t writer;
            mpack_writer_init_growable(&writer, &data, &size);

            mpack_start_map(&writer, 2);
            mpack_write_cstr(&writer, "compact");
            mpack_write_bool(&writer, true);
            mpack_write_cstr(&writer, "schema");
            mpack_write_uint(&writer, 0);
            mpack_finish_map(&writer);

            if (mpack_writer_destroy(&writer) != mpack_ok) {
                printf("Error encoding data, :(\n");
            }

            netcode_client_send_packet(client, data, size);
            free(data);

            //netcode_client_send_packet(client, packet_data, NETCODE_MAX_PACKET_SIZE);
        }

        while (1) {
            int packet_bytes;
            uint64_t packet_sequence;
            // does recieve packet do the full process for verifying the packet?
            // is packet_bytes just what I want?
            // I think this is like asserting that the packet I'm getting is what I sent because
            // the 
            void * packet = netcode_client_receive_packet(client, &packet_bytes, &packet_sequence);
            if (!packet)
                break;
            (void) packet_sequence;
            assert(packet_bytes == NETCODE_MAX_PACKET_SIZE);
            assert(memcmp(packet, packet_data, NETCODE_MAX_PACKET_SIZE) == 0);
            netcode_client_free_packet(client, packet);
        }

        if (netcode_client_state(client) <= NETCODE_CLIENT_STATE_DISCONNECTED) {};

        BeginDrawing();

        ClearBackground(RAYWHITE);
        DrawFPS(GetScreenWidth() - 75, 10);

        //int frack = GuiButton((Rectangle) { 10, 10, GetScreenWidth()-10, GetScreenHeight()-10 }, FormatText("Hello World: %s", "fracker"));
        //if (frack) {
            //printf("U pressed it\n");

            //int hello_packet_size = NETCODE_MAX_PACKET_SIZE;
            //static char hello_packet[NETCODE_MAX_PACKET_SIZE] = "Hello";

            //netcode_client_send_packet(client, hello_packet, sizeof(hello_packet));
        //}

        EndDrawing();

        time += delta_time;
    }

    printf( "\nshutting down\n" );

    netcode_client_destroy(client);

    netcode_term();
    CloseWindow();
    return 0;
}
