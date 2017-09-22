#include "wizard.h"
#include "wizard_network.h"
#include "wizard_message.h"
#include "wizard_simulation.h"

#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

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

    SimulationState state = create_new_one_player_game();

/*     Camera2D camera;
    camera.target = (Vector2) { 0, 0 };
    camera.offset = (Vector2) {GetScreenWidth()/2, GetScreenHeight()/2};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f; */

    while (!WindowShouldClose()) {
        client_update(&client, time);

        /* if (IsKeyPressed(KEY_ONE)) {
            camera.zoom += 1;
            printf("Zoom: %f\n", camera.zoom);
        }
        if (IsKeyPressed(KEY_TWO)) {
            camera.zoom -= 1;
            printf("Zoom: %f\n", camera.zoom);
        } */

        uint8_t *packet;
        size_t packet_size;
        uint64_t packet_sequence;
        while ((packet = client_packet_receive(&client, &packet_size, &packet_sequence))) {
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
        simulation_step(&state, &next_state, &frame_input, (float)delta_time);
        state = next_state;

        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Without any camera, lets draw the bounds of the world where we want them.
        V2 world_min = {.x = 0, .y = 0};
        V2 world_max = {.x = WORLD_WIDTH, .y = WORLD_HEIGHT};

        // scale so something at max is drawn at edge of screen
        // make    scale * max = width
        // scale = width / max
        V2 scale = {
            .x = GetScreenWidth() / world_max.x,
            .y = GetScreenHeight() / world_max.y
        };

        #define WORLD(v) (Vector2){v.x * scale.x, GetScreenHeight() - v.y * scale.y}

        for (int y = 0; y < WORLD_HEIGHT; y++) {
            // horisontal line at y
            V2 from = (V2){world_min.x, y};
            V2 to = (V2){world_max.x, y};
            DrawLineEx(WORLD(from), WORLD(to), 1.0, BLUE);
        }

        for (int x = 0; x < WORLD_WIDTH; x++) {
            // horisontal line at y
            V2 from = (V2){x, world_min.y};
            V2 to = (V2){x, world_max.y};
            DrawLineEx(WORLD(from), WORLD(to), 1.0, BLUE);
        }

        V2 a = world_min;
        V2 b = (V2){world_min.x, world_max.y};
        V2 c = world_max;
        V2 d = (V2){world_max.x, world_min.y};

        DrawLineEx(WORLD(a), WORLD(b), 5.0, GREEN);
        DrawLineEx(WORLD(b), WORLD(c), 5.0, GREEN);
        DrawLineEx(WORLD(c), WORLD(d), 5.0, GREEN);
        DrawLineEx(WORLD(d), WORLD(a), 5.0, GREEN);
        
        DrawFPS(GetScreenWidth() - 80, 10);
        DrawText(FormatText("(%.1f,%.1f)", state.entities[0].p.x, state.entities[0].p.y), GetScreenWidth()-100, 30, 24, RED);

        V2 player_p = (V2){state.entities[0].p.x, state.entities[0].p.y};
        Vector2 world_player_p = WORLD(player_p);
        world_player_p.x -= 0.3*scale.x;
        world_player_p.y -= scale.y;

        V2 player_size = (V2){0.3*2, 1};
        //DrawRectangle(WORLD(player_min).x, WORLD(player_min).y, WORLD(player_max).x, WORLD(player_max).y, BLUE);
        DrawRectangleV(world_player_p, (Vector2){0.3*2*scale.x, 1*scale.y}, BLUE);

        //DrawRectangle(state.entities[0].dp.x * draw_scale - 0.3*draw_scale, GetScreenHeight()/2 - state.entities[0].dp.y * draw_scale - 1*draw_scale, 0.3*2 * draw_scale, 1 * draw_scale, BLUE);
        //DrawCircle(0,0,10,RED);

        // bounds of world are suppossed to be 0,0 and 100,100

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
