#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <malloc.h>

#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "wizard_network.h"
#include "wizard_message.h"

// @TODO: Make it not a console app for windows release mode.
int main(int argc, char**argv ) {
    // raylib
    int screen_width = 1024;
    int screen_height = 768;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screen_width, screen_height, "Wizard Game");
    SetTargetFPS(60);

    NetworkClient client = (NetworkClient){0};

    double time = 0.0;
    double delta_time = 1.0 / 60.0;

    client_connect(&client, time, NULL);

    while (!WindowShouldClose()) {
        client_update(&client, time);

        // @TODO: state stuff in our client
        // @TODO: Learn the netcode client lifecycle
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
            printf("We got a packet!\n");
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
