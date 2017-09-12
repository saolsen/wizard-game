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

#include "wizard_network.h"

// @TODO: Share this with server and client for test.
/* static uint8_t private_key[NETCODE_KEY_BYTES] = { 0x60, 0x6a, 0xbe, 0x6e, 0xc9, 0x19, 0x10, 0xea, 
                                                  0x9a, 0x65, 0x62, 0xf6, 0x6f, 0x2b, 0x30, 0xe4, 
                                                  0x43, 0x71, 0xd6, 0x2c, 0xd1, 0x99, 0x27, 0x26,
                                                  0x6b, 0x3c, 0x60, 0xf4, 0xb7, 0x15, 0xab, 0xa1 }; */

// These are set in the reliable config.
// I think when you send "RELIABLE SEND PACKET"
// it uses this function, I'm not sure when the process function gets called tho.
void transmit_packet_function(void* _context, int index, uint16_t sequence, uint8_t *packet_data, int packet_bytes)
{
    struct netcode_client_t *client = (struct netcode_client_t*)_context;
    netcode_client_send_packet(client, packet_data, packet_bytes);
    // @NOTE: I think index is a thing we can use to keep track of the packet.
}

int process_packet_function(void* _context, int index, uint16_t sequence, uint8_t *packet_data, int packet_bytes)
{
    printf("Received Packet\n");
    return 0;
}

int main(int argc, char**argv ) {
    // raylib
    int screen_width = 1024;
    int screen_height = 768;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screen_width, screen_height, "Wizard Game");
    SetTargetFPS(60);

    Client client = (Client){0};

    double time = 0.0;
    double delta_time = 1.0 / 60.0;

    client_connect(&client, time, NULL);

    while (!WindowShouldClose()) {
        client_update(&client, time);

        // @TODO: state stuff in our client
        if (netcode_client_state(client.netcode_client) == NETCODE_CLIENT_STATE_CONNECTED) {
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

            client_packet_send(&client, data, (uint32_t)size);
            
            free(data);
        }

        uint8_t *packet;
        uint32_t packet_size;
        uint64_t packet_sequence;
        while (packet = client_packet_receive(&client, &packet_size, &packet_sequence)) {
            printf("We got a packet!");
        }

        //if (netcode_client_state(client) <= NETCODE_CLIENT_STATE_DISCONNECTED) {};

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
        
        client_send_packets(&client);
        time += delta_time;
    }

    printf( "\nshutting down\n" );

    client_destroy(&client);
    CloseWindow();
    return 0;
}
