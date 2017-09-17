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

typedef struct {
    union {
        float x;
        float y;
    };
    float e[2];
} V2;

typedef struct {
    uint8_t num_players;
    uint64_t player_ids[32];
    V2 player_positions[32];

} SimulationState;