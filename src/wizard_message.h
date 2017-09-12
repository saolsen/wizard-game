// Wizard messages send over the network between the clients and server.
#ifndef _wizard_message_h
#define _wizard_message_h

typedef enum {
    RELIABLE = 1,
} MessageFlags;

// @OPTOMIZE: Pack flags and type into single value.
typedef struct {
    MessageFlags flags;
    uint32_t size;
    uint8_t *data;
} Message;

// Messages
typedef struct {
    Message message;
    int which_button; // I dunno, like whatever maybe throw some state in there.
} PressButtonMessage;

// message_send_reliable(connection, data, size)
// message_send_unreliable(connection, data, size)

#endif //_wizard_message_h