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

typedef struct {
    MessageType type;
    int player_index;
    char *player_name;
} PlayerConnectedMessage;

typedef struct {
    MessageType type;
    int player_index;
} PlayerDisconnectedMessage;

typedef struct {
    MessageType type;
    float player_positions[32*2]; // x and y for all 32 possible players.
} CurrentPlayerStateMessage;

typedef struct {
    MessageType type;
    int player_index;
} PlayerWaveMessage;

// @Q: Is there a better way to do this? I could embed them all manually but that's annoying to remember which fields
// go in which. I could metaprogram that I guess.
typedef union {
    PlayerConnectedMessage _pcm;
    PlayerDisconnectedMessage _pdm;
    CurrentPlayerStateMessage _cpsm;
    PlayerWaveMessage _pwm;
} MessageStorage;


// @OPTIMIZATION: I could just metaprogram these functions from the struct definitions above.
int message_serialize(MessageStorage *message, uint8_t **data, size_t *size)
{   
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, data, size);

    switch(*(MessageType*)message) {
        case MT_Null:
            return 1;
        case MT_PlayerConnected: {
            PlayerConnectedMessage *msg = (PlayerConnectedMessage*)message;
            mpack_start_array(&writer, 3);
            mpack_write_int(&writer, msg->type);
            mpack_write_int(&writer, msg->player_index);
            mpack_write_cstr(&writer, msg->player_name);
            mpack_finish_array(&writer);
        } break;
        case MT_PlayerDisconnected: {
            PlayerDisconnectedMessage *msg = (PlayerDisconnectedMessage*)message;
            mpack_start_array(&writer, 2);
            mpack_write_int(&writer, msg->type);
            mpack_write_int(&writer, msg->player_index);
            mpack_finish_array(&writer);
        } break;
        case MT_CurrentPlayerState: {
            CurrentPlayerStateMessage *msg = (CurrentPlayerStateMessage*)message;
            mpack_start_array(&writer, 2);
            mpack_write_int(&writer, msg->type);
            mpack_start_array(&writer, 32 * 2);
            for (int i=0; i<32*2; i++) {
                mpack_write_float(&writer, msg->player_positions[i]);
            }
            mpack_finish_array(&writer);
            mpack_finish_array(&writer);
        } break;
        case MT_PlayerWave: {
            PlayerWaveMessage *msg = (PlayerWaveMessage*)message;
            mpack_start_array(&writer, 2);
            mpack_write_int(&writer, msg->type);
            mpack_write_int(&writer, msg->player_index);
            mpack_finish_array(&writer);
        } break;
    }

    if (mpack_writer_destroy(&writer) != mpack_ok) {
        printf("Error encoding data, :(\n");
        return 1;
    }

    return 0;
}

MessageType message_deserialize(uint8_t *data, size_t size, MessageStorage *message)
{
    mpack_reader_t reader;
    mpack_reader_init_data(&reader, data, size);

    mpack_expect_array(&reader);
    MessageType *message_type = (MessageType*)message;
    *message_type = (MessageType)mpack_expect_int(&reader);

    switch(*message_type) {
        case MT_Null:
            return 1;
        case MT_PlayerConnected: {
            PlayerConnectedMessage *msg = (PlayerConnectedMessage*)message;
            msg->player_index = mpack_expect_int(&reader);
            msg->player_name = mpack_expect_cstr_alloc(&reader, 1024);
        } break;
        case MT_PlayerDisconnected: {
            PlayerDisconnectedMessage *msg = (PlayerDisconnectedMessage*)message;
            msg->player_index = mpack_expect_int(&reader);
        } break;
        case MT_CurrentPlayerState: {
            CurrentPlayerStateMessage *msg = (CurrentPlayerStateMessage*)message;
            mpack_expect_array(&reader);
            for (int i=0; i<32*2; i++) {
                msg->player_positions[i] = mpack_expect_float(&reader);
            }
            mpack_done_array(&reader);
        } break;
        case MT_PlayerWave: {
            PlayerWaveMessage *msg = (PlayerWaveMessage*)message;
            msg->player_index = mpack_expect_int(&reader);
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
        PlayerConnectedMessage message = {.type = MT_PlayerConnected, .player_index=12, .player_name="Steben"};
        message_serialize((MessageStorage*)&message, &data, &s);
        PlayerConnectedMessage result;
        MessageType mt = message_deserialize(data, s, (MessageStorage*)&result);
        ASSERT_EQ(message.type, result.type);
        ASSERT_EQ(message.player_index, result.player_index);
        ASSERT_FALSE(strcmp(message.player_name, result.player_name));
        free(data); // @TODO: figure out how you're really gonna manage memory.
    }
    {
        PlayerDisconnectedMessage message = {.type = MT_PlayerDisconnected, .player_index=100};
        message_serialize((MessageStorage*)&message, &data, &s);
        PlayerDisconnectedMessage result;
        MessageType mt = message_deserialize(data, s, (MessageStorage*)&result);
        ASSERT_EQ(message.type, result.type);
        ASSERT_EQ(message.player_index, result.player_index);
        free(data);
    }
    {
        CurrentPlayerStateMessage message = {
            .type = MT_CurrentPlayerState,
            .player_positions = {1}
        };
        message_serialize((MessageStorage*)&message, &data, &s);
        CurrentPlayerStateMessage result;
        MessageType mt = message_deserialize(data, s, (MessageStorage*)&result);
        ASSERT_EQ(message.type, result.type);
        for (int i=0; i<32*2; i++) {
            ASSERT_EQ(message.player_positions[i], result.player_positions[i]);
        }
        free(data);
    }
    {
        PlayerWaveMessage message = {
            .type = MT_PlayerWave,
            .player_index = 44
        };
        message_serialize((MessageStorage*)&message, &data, &s);
        PlayerWaveMessage result;
        MessageType mt = message_deserialize(data, s, (MessageStorage*)&result);
        ASSERT_EQ(message.type, result.type);
        ASSERT_EQ(message.player_index, result.player_index);
        free(data);
    }
    PASS();
}

SUITE(wizard_message_tests) {
    RUN_TEST(test_serialize_deserialize);
}
#endif