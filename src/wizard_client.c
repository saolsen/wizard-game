#include "netcode.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <inttypes.h>

#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define CONNECT_TOKEN_EXPIRY 30
#define CONNECT_TOKEN_TIMEOUT 5
#define PROTOCOL_ID 0x1122334455667788

static volatile int quit = 0;

void interrupt_handler( int signal )
{
    (void) signal;
    quit = 1;
}

static uint8_t private_key[NETCODE_KEY_BYTES] = { 0x60, 0x6a, 0xbe, 0x6e, 0xc9, 0x19, 0x10, 0xea, 
                                                  0x9a, 0x65, 0x62, 0xf6, 0x6f, 0x2b, 0x30, 0xe4, 
                                                  0x43, 0x71, 0xd6, 0x2c, 0xd1, 0x99, 0x27, 0x26,
                                                  0x6b, 0x3c, 0x60, 0xf4, 0xb7, 0x15, 0xab, 0xa1 };

int main( int argc, char ** argv )
{
    // raylib
    int screen_width = 1024;
    int screen_height = 768;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screen_width, screen_height, "Wizard Game");
    SetTargetFPS(60);

    if ( netcode_init() != NETCODE_OK )
    {
        printf( "error: failed to initialize netcode.io\n" );
        return 1;
    }

    netcode_log_level( NETCODE_LOG_LEVEL_INFO );

    double time = 0.0;
    double delta_time = 1.0 / 60.0;

    printf( "[client]\n" );

    struct netcode_client_t * client = netcode_client_create( "0.0.0.0", time );

    if ( !client )
    {
        printf( "error: failed to create client\n" );
        return 1;
    }

    NETCODE_CONST char * server_address = ( argc != 2 ) ? "127.0.0.1:40000" : argv[1];        

    uint64_t client_id = 0;
    netcode_random_bytes( (uint8_t*) &client_id, 8 );
    printf( "client id is %.16" PRIx64 "\n", client_id );

    uint8_t connect_token[NETCODE_CONNECT_TOKEN_BYTES];

    if ( netcode_generate_connect_token( 1, &server_address, CONNECT_TOKEN_EXPIRY, CONNECT_TOKEN_TIMEOUT, client_id, PROTOCOL_ID, 0, private_key, connect_token ) != NETCODE_OK )
    {
        printf( "error: failed to generate connect token\n" );
        return 1;
    }

    netcode_client_connect( client, connect_token );

    signal( SIGINT, interrupt_handler );

    uint8_t packet_data[NETCODE_MAX_PACKET_SIZE];
    int i;
    for ( i = 0; i < NETCODE_MAX_PACKET_SIZE; ++i )
        packet_data[i] = (uint8_t) i;

    while (!WindowShouldClose())
    {
        netcode_client_update( client, time );

        if ( netcode_client_state( client ) == NETCODE_CLIENT_STATE_CONNECTED )
        {
            netcode_client_send_packet( client, packet_data, NETCODE_MAX_PACKET_SIZE );
        }

        while ( 1 )             
        {
            int packet_bytes;
            uint64_t packet_sequence;
            void * packet = netcode_client_receive_packet( client, &packet_bytes, &packet_sequence );
            if ( !packet )
                break;
            (void) packet_sequence;
            assert( packet_bytes == NETCODE_MAX_PACKET_SIZE );
            assert( memcmp( packet, packet_data, NETCODE_MAX_PACKET_SIZE ) == 0 );            
            netcode_client_free_packet( client, packet );
        }

        if ( netcode_client_state( client ) <= NETCODE_CLIENT_STATE_DISCONNECTED )
            break;

        BeginDrawing();

        ClearBackground(RAYWHITE);
        DrawFPS(GetScreenWidth() - 250, 10);

        int frack = GuiButton((Rectangle) { 10, 10, 400, 200 }, FormatText("Hello World: %s", "fracker"));
        if (frack) {
            printf("U pressed it\n");

            //int hello_packet_size = NETCODE_MAX_PACKET_SIZE;
            static char hello_packet[NETCODE_MAX_PACKET_SIZE] = "Hello";

            netcode_client_send_packet(client, hello_packet, sizeof(hello_packet));
        }

        EndDrawing();

        time += delta_time;
    }

    printf( "\nshutting down\n" );

    netcode_client_destroy(client);

    netcode_term();
    CloseWindow();
    return 0;
}
