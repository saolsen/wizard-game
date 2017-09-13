#ifndef _wizard_message_h
#define _wizard_message_h

// gonna use mpack because it should make this a lot easier.
// @Optomize: Something with better compression for our specific use cases.
#include "mpack.h"

typedef enum {
    PlayerConnected,      // Sent by the server to all connected clients when someone joins the game.
    PlayerDisconnected,   // Sent by the server to all connected clients when someone leaves the game.
    CurrentPlayerState,   // Sent regulary by the server to update clients on players positions.
    Wave,                 // Sent by the server to all connected clients when someone waves.
} MessageType;

typedef struct {
    MessageType type;
    int player_index;
    char *player_name;
} PlayerConnectedMessage;

// Serializes msg, data will be the serialized version and size will be it's size.
// @Q: How do we handle the memory needed for this kind of stuff.
// Need to really give that some thought.
// If I do like a message union thing I could fill in a statically allocated message and serialize it. That's a neat option.
// if serialization was tied to sending of messages too this could be neater with memory.
// maybe even macros that take a struct initializer? Lets me do defaults and stuff too?

// We're gonna use msgpack arrays, it's actually a little overkill because we know how many elements we have always
// but whatever, it lets us use this lib and can generically unserialize stuff if we want to also.
int message_player_connected_serialize(PlayerConnectedMessage msg, uint8_t **data, size_t *size)
{
    // Gonna start by encoding shit as maps. This is not efficiant but could be really nice if I want to maybe visualize some of this shit
    // in flight? I dunno, @TODO: look into direct struct encoding.
    mpack_writer_t writer;
    mpack_writer_init_growable(&writer, data, size);

    mpack_start_array(&writer, 3);
    mpack_write_int(&writer, msg.type);
    mpack_write_int(&writer, msg.player_index);
    mpack_write_cstr(&writer, msg.player_name);
    mpack_finish_array(&writer);

    if (mpack_writer_destroy(&writer) != mpack_ok) {
        printf("Error encoding data, :(\n");
    }

    //client_packet_send(&client, data, (uint32_t)size);
    
    // when do we free(data)?
    return 0;
}

PlayerConnectedMessage message_player_connected_deserialize(uint8_t *data, size_t size)
{
    PlayerConnectedMessage result;

    mpack_reader_t reader;
    mpack_reader_init_data(&reader, data, size);

    mpack_expect_array(&reader);
    result.type = mpack_expect_int(&reader);
    result.player_index = mpack_expect_int(&reader);
    result.player_name = mpack_expect_cstr_alloc(&reader, 1024);
    mpack_done_array(&reader);

    mpack_reader_destroy(&reader);

    return result;
}


#endif //_wizard_message_h

#ifdef WIZARD_TESTING

TEST test_serialize_deserialize(void) {
    PlayerConnectedMessage message = {.type = PlayerConnected, .player_index=12, .player_name="Steben"};
    uint8_t *data = NULL;
    size_t s;
    message_player_connected_serialize(message, &data, &s);
    PlayerConnectedMessage result = message_player_connected_deserialize(data, s);
    ASSERT_EQ(message.type, result.type);
    ASSERT_EQ(message.player_index, result.player_index);
    ASSERT_FALSE(strcmp(message.player_name, result.player_name));
    free(data); // @TODO: figure out how you're really gonna manage memory.
    PASS();
}

SUITE(wizard_message_tests) {
    RUN_TEST(test_serialize_deserialize);
}
#endif