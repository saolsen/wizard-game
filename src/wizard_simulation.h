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

// need player input.
// I guess before this is run player input must be figured out for other clients where we don't know wuddup.

// In c mutation is def the nicest thing to do code wise. If I can just mutate the fuck out of this prev state it's
// chill. Maybe I just need to literally pass in old and mutate it and handle copying behind the scenes.

// What happens if we do it in the functional style like this
void simulation_step(const SimulationState *prev, SimulationState *next, const PlayerInput *inputs, float dt) {
    // @TODO: Maybe have this already happen before calling this.
    *next = *prev;
    
    for (int player_index=0; player_index < next->num_players; player_index++) {
        uint64_t player_id = next->player_ids[player_index];
        const PlayerInput *input = inputs + player_index;
        
        V2 *player_pos = next->player_positions + player_index;

        if (input->pressing_forward) {
            player_pos->y += 1; // lol
        }
    }
}

#endif // _wizard_simulation_h
#ifdef WIZARD_TESTING

TEST test_function_runs(void) {
    SimulationState prev = {0};
    SimulationState next = {0};
    simulation_step(&prev, &next, NULL, 1.0f/60.0f);
    ASSERT(1);
    PASS();
}

TEST test_1_player_simulation(void) {
    SimulationState prev = {
        .num_players = 1,
        .player_ids = {12345, 0},
        .player_positions = {10, 10, 0}
    };
    PlayerInput player_inputs[] = {
        (PlayerInput){0}
    };
    SimulationState next = {0};
    simulation_step(&prev, &next, player_inputs, 1.0f/60.0f);
    ASSERT(next.num_players = prev.num_players);
    PASS();
}

SUITE(wizard_simulation_tests) {
    RUN_TEST(test_function_runs);
    RUN_TEST(test_1_player_simulation);
}
#endif