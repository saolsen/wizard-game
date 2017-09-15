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

void _state_change_callback(void* , int, int);

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

    bool am_connected = false;

    while (!WindowShouldClose()) {
        client_update(&client, time);

        // I don't think I really need this, but I do probably need to be sending some sort of message every frame
        // to keep the connection alive.

        /* if (am_connected) {
            // keepalive packet, gonna ditch this.

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
        } */

        uint8_t *packet;
        uint32_t packet_size;
        uint64_t packet_sequence;
        while (packet = client_packet_receive(&client, &packet_size, &packet_sequence)) {
            if (packet_size != NETCODE_MAX_PACKET_SIZE) {
                MessageStorage message_storage;
                MessageType mt = message_deserialize(packet, packet_size, &message_storage);

                switch(mt) {
                    case MT_PlayerConnected: {
                        PlayerConnectedMessage *message = (PlayerConnectedMessage*)&message_storage;
                        printf("[msg] Client '%.16" PRIx64 "' has connected\n", message->player_id);
                    } break;
                    case MT_PlayerDisconnected: {
                        PlayerDisconnectedMessage *message = (PlayerDisconnectedMessage*)&message_storage;
                        printf("[msg] Client '%.16" PRIx64 "' has disconnected\n", message->player_id);
                    } break;
                    case MT_CurrentPlayerState: {
                        CurrentPlayerStateMessage *message = (CurrentPlayerStateMessage*)&message_storage;
                        // @TODO
                    } break;
                    case MT_PlayerWave: {
                        PlayerWaveMessage *message = (PlayerWaveMessage*)&message_storage;
                        printf("[msg] Client '%.16" PRIx64 "' waves\n", message->player_id);
                    }
                    default: break;
                }
            }
            client_packet_free(&client);
        }

        //if (netcode_client_state(client) <= NETCODE_CLIENT_STATE_DISCONNECTED) {};

        BeginDrawing();

        ClearBackground(RAYWHITE);
        DrawFPS(GetScreenWidth() - 75, 10);

        int frack = GuiButton((Rectangle) { 10, 10, GetScreenWidth()-20, GetScreenHeight()-20 }, FormatText("Press to %s", "wave"));
        if (frack) {
            printf("U pressed wave\n");
            
            PlayerWaveMessage m = {
                .type = MT_PlayerWave,
                .player_id = client.client_id
            };

            uint8_t *packet;
            uint32_t packet_size;
            message_serialize((MessageStorage*)&m, &packet, &packet_size);
            client_packet_send(&client, packet, packet_size);
        }

        EndDrawing();
        
        client_send_packets(&client);
        time += delta_time;
    }

    printf( "\nshutting down\n" );

    client_destroy(&client);
    CloseWindow();
    return 0;
}
