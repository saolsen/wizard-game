#ifndef _wizard_message_h
#define _wizard_message_h

// Gonna use mpack because it should make this a lot easier.
// @Optomize: Something with better compression for our specific use cases.
#include "mpack.h"

typedef enum {
    MT_Null,
    MT_PlayerConnected,      // Sent by the server to all connected clients when someone joins the game.
    MT_PlayerDisconnected,   // Sent by the server to all connected clients when someone leaves the game.
    MT_CurrentPlayerState,   // Sent regulary by the server to update clients on players positions.
    MT_PlayerWave,           // Sent by the server to all connected clients when someone waves.
} MessageType;

/* typedef struct {
    MessageType type;
    uint64_t player_id;
} PlayerConnectedMessage;

typedef struct {
    MessageType type;
    uint64_t player_id;
} PlayerDisconnectedMessage;

typedef struct {
    MessageType type;
    uint64_t player_ids[32];
    int num_players;
    float player_positions[32*2]; // x and y for all 32 possible players.
} CurrentPlayerStateMessage;

typedef struct {
    MessageType type;
    uint64_t player_id;
} PlayerWaveMessage; */

// If I figure out how I wanna do memory management I can just have these all
// be their own struct and just pass header pointers or something.

// @Q: Is there a better way to do this? I could embed them all manually but that's annoying to remember which fields
// go in which. I could metaprogram that I guess.


// @NOTE: Keep all the messages in the same structure to make it easy to work with them. Usually only a few of these are allocated
// at a time so it's not a big deal. The over the wire format is compressed (see serialize / deserialize).

// If I decide I can just pass an arena for messages and just allocate them on demand I can go back to 1 struct per mesasge type
// and a switch on the type. That would actually be really nice to do. Can do dynamic sized shit that way too.
typedef struct {
    MessageType type;
    union {
        // Player Connected, Player Disconnected, Player Wave
        struct {
            uint64_t player_id;
        };
        // Current Player State
        struct {
            uint64_t player_ids[32];
            int num_players;
            float player_positions[32*2];
        };
    };
} Message;


// @OPTIMIZATION: I could just metaprogram these functions from the struct definitions above.
int message_serialize(Message *message, uint8_t **data, size_t *size)
{   
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, (void*)data, size);

    Message *msg = message;
    switch(message->type) {
        case MT_Null:
            return 1;
        case MT_PlayerConnected: {
            mpack_start_array(&writer, 2);
            mpack_write_int(&writer, msg->type);
            mpack_write_u64(&writer, msg->player_id);
            mpack_finish_array(&writer);
        } break;
        case MT_PlayerDisconnected: {
            mpack_start_array(&writer, 2);
            mpack_write_int(&writer, msg->type);
            mpack_write_u64(&writer, msg->player_id);
            mpack_finish_array(&writer);
        } break;
        case MT_CurrentPlayerState: {
            mpack_start_array(&writer, 3);
            mpack_write_int(&writer, msg->type);
            mpack_start_array(&writer, msg->num_players);
            for (int i=0; i<msg->num_players; i++) {
                mpack_write_u64(&writer, msg->player_ids[i]);
            }
            mpack_finish_array(&writer);
            mpack_start_array(&writer, msg->num_players*2);
            for (int i=0; i<msg->num_players*2; i++) {
                mpack_write_float(&writer, msg->player_positions[i]);
            }
            mpack_finish_array(&writer);
            mpack_finish_array(&writer);
        } break;
        case MT_PlayerWave: {
            mpack_start_array(&writer, 2);
            mpack_write_int(&writer, msg->type);
            mpack_write_u64(&writer, msg->player_id);
            mpack_finish_array(&writer);
        } break;
    }

    if (mpack_writer_destroy(&writer) != mpack_ok) {
        printf("Error encoding data, :(\n");
        return 1;
    }

    return 0;
}

MessageType message_deserialize(uint8_t *data, size_t size, Message *message)
{
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, data, size);

    mpack_expect_array(&reader);
    MessageType *message_type = (MessageType*)message;
    *message_type = (MessageType)mpack_expect_int(&reader);

    Message *msg = message;
    switch(*message_type) {
        case MT_Null:
            return 1;
        case MT_PlayerConnected: {
            msg->player_id = mpack_expect_u64(&reader);
        } break;
        case MT_PlayerDisconnected: {
            msg->player_id = mpack_expect_u64(&reader);
        } break;
        case MT_CurrentPlayerState: {
            msg->num_players = mpack_expect_array(&reader);
            for (int i=0; i<msg->num_players; i++) {
                msg->player_ids[i] = mpack_expect_u64(&reader);
            }
            mpack_done_array(&reader);
            mpack_expect_array(&reader);
            for (int i=0; i<msg->num_players*2; i++) {
                msg->player_positions[i] = mpack_expect_float(&reader);
            }
            mpack_done_array(&reader);
        } break;
        case MT_PlayerWave: {
            msg->player_id = mpack_expect_u64(&reader);
        } break;
    }
    
    mpack_done_array(&reader);
    mpack_reader_destroy(&reader);

    return *message_type;
}
// @Q: Is there a way to do the isreading/iswriting thing with msgpack so they don't get out of sync?

#endif //_wizard_message_h

#ifdef WIZARD_TESTING

// @TODO: Make this a property test.
TEST test_serialize_deserialize(void) {
    uint8_t *data = NULL;
    size_t s;
    {
        Message message = {.type = MT_PlayerConnected, .player_id=12};
        message_serialize(&message, &data, &s);
        Message result;
        MessageType mt = message_deserialize(data, s, &result);
        ASSERT_EQ(message.type, result.type);
        ASSERT_EQ(message.player_id, result.player_id);
        free(data); // @TODO: figure out how you're really gonna manage memory.
    }
    {
        Message message = {.type = MT_PlayerDisconnected, .player_id=100};
        message_serialize(&message, &data, &s);
        Message result;
        MessageType mt = message_deserialize(data, s, &result);
        ASSERT_EQ(message.type, result.type);
        ASSERT_EQ(message.player_id, result.player_id);
        free(data);
    }
    {
        Message message = {
            .type = MT_CurrentPlayerState,
            .num_players = 1,
            .player_ids = {123, 0},
            .player_positions = {1, 1, 0}
        };
        message_serialize(&message, &data, &s);
        Message result;
        MessageType mt = message_deserialize(data, s, &result);
        ASSERT_EQ(message.type, result.type);
        ASSERT_EQ(message.num_players, result.num_players);
        for (int i=0; i<message.num_players; i++) {
            ASSERT_EQ(message.player_ids[i], result.player_ids[i]);
        }
        for (int i=0; i<message.num_players*2; i++) {
            ASSERT_EQ(message.player_positions[i], result.player_positions[i]);
        }
        free(data);
    }
    {
        Message message = {
            .type = MT_PlayerWave,
            .player_id = 44
        };
        message_serialize(&message, &data, &s);
        Message result;
        MessageType mt = message_deserialize(data, s, &result);
        ASSERT_EQ(message.type, result.type);
        ASSERT_EQ(message.player_id, result.player_id);
        free(data);
    }
    PASS();
}

SUITE(wizard_message_tests) {
    RUN_TEST(test_serialize_deserialize);
}
#endif