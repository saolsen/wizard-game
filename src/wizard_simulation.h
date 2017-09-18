// This is the code for the game simulation.
// Step 1
// - write some simulation code, run it with local client inputs and render the result.

// Step 2
// - Send inputs to server who also runs simulation. Server sends authoritative results.
// - Rollback to server authoritative state and rerun simulation locally to now, then interpolate
//   what the client thought was right to what is right quickly.

// Step 3
// - client side predict what other clients input would be and use that in speculative simulation.

// So like, this is the kind of thing where persistent data structures are so dope.
// cuz like, i'm going to keep multiple versions of this in memory that are all going to be almost exactly the same.
// @TODO: think about that...

#ifndef _wizard_simulation_h
#define _wizard_simulation_h

typedef struct {
    union {
        float x;
        float y;
    };
    float e[2];
} V2;

// Need my and other players inputs for this frame.
// Need to know the time stuff happens at.
typedef struct {
    int pressing_forward;
    int pressing_backwards;
    int pressing_left;
    int pressing_right;
    // Need like a way to keep track of aiming / facing
    // that could be a joystick value, or mouse motion I guess.
    // if it's not true top down, player on the bottom has an advantage if the camera is fixed.

} PlayerInput;

typedef struct {
    // Need some sort of time value
    // @TODO: How do we deal with time in this?
    uint8_t num_players;
    uint64_t player_ids[32];
    V2 player_positions[32];

} SimulationState;

void simulation_step(SimulationState *prev, SimulationState *next, float dt) {
    // no op
    *next = *prev;
}

#endif // _wizard_simulation_h
#ifdef WIZARD_TESTING

TEST test_function_runs(void) {
    SimulationState prev = {0};
    SimulationState next = {0};
    simulation_step(&prev, &next, 1.0f/60.0f);
    ASSERT(1);
    PASS();
}

SUITE(wizard_simulation_tests) {
    RUN_TEST(test_something);
}
#endif