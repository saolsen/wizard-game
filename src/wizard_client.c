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
#include "wizard_simulation.h"

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

    SimulationState state = {
        .num_players = 1,
        .player_ids = {12345, 0},
        .player_positions = {(V2){10, 10}, 0},
        .player_velocities = {(V2){0, 0}, 0}
    };

    while (!WindowShouldClose()) {
        client_update(&client, time);

        uint8_t *packet;
        size_t packet_size;
        uint64_t packet_sequence;
        while (packet = client_packet_receive(&client, &packet_size, &packet_sequence)) {
            if (packet_size != NETCODE_MAX_PACKET_SIZE) {
                Message message;
                MessageType mt = message_deserialize(packet, packet_size, &message);

                switch(mt) {
                    case MT_PlayerConnected: {
                        printf("[msg] Client '%.16" PRIx64 "' has connected\n", message.player_id);
                    } break;
                    case MT_PlayerDisconnected: {
                        printf("[msg] Client '%.16" PRIx64 "' has disconnected\n", message.player_id);
                    } break;
                    case MT_CurrentPlayerState: {
                        // @TODO
                    } break;
                    case MT_PlayerWave: {
                        printf("[msg] Client '%.16" PRIx64 "' waves\n", message.player_id);
                    }
                    default: break;
                }
            }
            client_packet_free(&client);
        }

        PlayerInput frame_input = {
            .pressing_up = IsKeyDown(KEY_W),
            .pressing_down = IsKeyDown(KEY_S),
            .pressing_left = IsKeyDown(KEY_A),
            .pressing_right = IsKeyDown(KEY_D)
        };

        SimulationState next_state = state;
        simulation_step(&state, &next_state, &frame_input, delta_time);
        state = next_state;

        BeginDrawing();

        ClearBackground(RAYWHITE);
        DrawFPS(GetScreenWidth() - 75, 10);

        // @TODO: Draw the gamestate.
        DrawRectangle(state.player_positions[0].x - 5, GetScreenHeight() - state.player_positions[0].y - 10, 10, 10, BLUE);

        /* int frack = GuiButton((Rectangle) { 10, 10, GetScreenWidth()-20, GetScreenHeight()-20 }, FormatText("Press to %s", "wave"));
        if (frack) {
            printf("U pressed wave\n");
            
            Message m = {
                .type = MT_PlayerWave,
                .player_id = client.client_id
            };

            uint8_t *packet;
            size_t packet_size;
            message_serialize(&m, &packet, &packet_size);
            client_packet_send(&client, packet, packet_size);
        } */

        EndDrawing();
        
        client_send_packets(&client);
        time += delta_time;
    }

    printf( "\nshutting down\n" );

    client_destroy(&client);
    CloseWindow();
    return 0;
}
