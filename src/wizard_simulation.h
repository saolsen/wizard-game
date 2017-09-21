// clients send last n inputs so we dont need to make those messages reliable.
// server just sends the latest state every frame.

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

#include "wizard.h"

typedef union {
    struct {
        float x;
        float y;
    };
    float e[2];
} V2;

// Not data oriented at this point. Gonna probably start with a huge array like this and work it back? Maybe?
// @TODO: Figure out later, just doing this for now cuz it's easier?
typedef enum {
    ET_NONE,
    ET_Player,
    ET_WALL
} EntityType;

typedef enum {
    GT_NONE,
    GT_AABB,
    GT_CIRCLE,
} GeometryType;

// @NOTE: Geometry is all relative to the player position. Keep that one in mind?
// @Q: Should it be?

typedef struct {
    V2 p;
    V2 min;
    V2 max;
} AABB;

typedef struct {
    V2 pos;
    float r;
} Circle;

typedef struct {
    GeometryType type;
    union {
        AABB aabb;
        Circle circle;
    };
} Geometry;

typedef enum {
    EF_Collides = 0x1
    // EF_Active = 0x2
    // EF_FOO = 0x4
} EntityFlags;

// @NOTE: Reusable for any flags stuff.
// Dunno if I want this or just like use the shit.
bool flags_is_set(uint32_t flags, uint32_t flag)
{
    return flags & flag;
}

void flags_set(uint32_t *flags, uint32_t flag)
{
    *flags |= flag;
}

void flags_clear(uint32_t *flags, uint32_t flag)
{
    *flags &= ~flag;
}

typedef struct _entity {
    uint32_t entity_id;
    uint32_t flags;

    EntityType type;
    V2 p;   // position
    V2 dp;  // velocity
    V2 ddp; // acceleration
    Geometry collision_pieces[32]; // collision geometry positions are relative to entity position.
    uint8_t num_collision_pieces;

    struct _entity *next;
} Entity;

// Need my and other players inputs for this frame.
// Need to know the time stuff happens at.
typedef struct {
    int pressing_up;
    int pressing_down;
    int pressing_left;
    int pressing_right;
} PlayerInput;

typedef struct {
    // Need some sort of time value
    // @TODO: How do we deal with time in this?
    uint8_t num_players;
    uint64_t player_ids[32];
    uint32_t player_entities[32]; // The entity ids for the players.
    
    Entity entities[128];
    uint8_t num_entities;
    Entity *first_free_entity;
} SimulationState;

SimulationState create_new_one_player_game() {
    Entity basic_player_entity = {
        .entity_id = 99,
        .flags = EF_Collides,
        .type = ET_Player,
        .p = {.x=0, .y=0},
        .dp = {.x=0, .y=0},
        .ddp = {.x=0, .y=0},
        .collision_pieces = {
            {.type = GT_AABB,
            .aabb = {
                .p = {.x=0,.y=0},
                .min = {.x=-5, .y=0},
                .max = {.x=5, .y=10}}},
            0
        },
        .num_collision_pieces = 1,
    };

    SimulationState new_one_player_game = {
        .num_players = 1,
        .player_ids = {1, 0},
        .player_entities = {99, 0},
        .entities = {basic_player_entity,  0},
        .first_free_entity = NULL,
        .num_entities = 1
    };

    return new_one_player_game;
}

// need player input.
// I guess before this is run player input must be figured out for other clients where we don't know wuddup.

// In c mutation is def the nicest thing to do code wise. If I can just mutate the fuck out of this prev state it's
// chill. Maybe I just need to literally pass in old and mutate it and handle copying behind the scenes.

// What happens if we do it in the functional style like this
// What are all the outputs of this, are there additional messages that are generated to get sent to players
// and clients.
// How do we handle who is authoritative over stuff and if this is client or server code?

#define MAX_SPEED 250
#define FRICTION 10
#define WORLD_WIDTH 100
#define WORLD_HEIGHT 100

void simulation_step(const SimulationState *prev, SimulationState *next, const PlayerInput *inputs, float dt) {
    // Assumes that next is the same as prev so we only have to write changes.
    // just set it in case for now since there's no way to check struct equality easily.
    *next = *prev;
    // @TODO: Should use a fixed timestep!
    // Apply player input to entities.
    for (int player_index=0; player_index < next->num_players; player_index++) {
        uint64_t player_id = next->player_ids[player_index];
        uint32_t entity_id = next->player_entities[player_index];
        
        const PlayerInput *input = inputs + player_index;

        // Get entity by id
        // @TODO: Probably going to have a hashmap of entity id at some point since this lookup happens a lot.
        Entity *player_entity = NULL;
        for (int i=0; i<next->num_entities; i++) {
            Entity *entity = next->entities + i;
            if (entity->type != ET_NONE) {
                if (entity->entity_id == entity_id) {
                    player_entity = entity;
                    break;
                }
            }
        }
        if (player_entity) {
            V2 frame_acceleration = {.x = 0, .y = 0};
            if (input->pressing_up) { frame_acceleration.y += 1; }
            if (input->pressing_down) { frame_acceleration.y -= 1; }
            if (input->pressing_left) { frame_acceleration.x -= 1; }
            if (input->pressing_right) { frame_acceleration.x += 1; }
            frame_acceleration.x *= MAX_SPEED;
            frame_acceleration.y *= MAX_SPEED;
            player_entity->ddp = frame_acceleration;
        }
    }

    // @TODO: Prolly run AI here.

    // Simulate Entities
    // @OPTIMIZE: This is a place where data oriented design could help cache.
    // Only need geometry and p/dp/ddp for these calculations I think.

    // Simulate entities one at a time?
    for (int i=0; i<next->num_entities; i++) {
        Entity *entity = next->entities + i;
        if (entity->ddp.x > 0) {
            printf("FUCK");
        }

        //entity->ddp.x += -FRICTION * entity->dp.x;
        //entity->ddp.y += -FRICTION * entity->dp.y;

        // @TODO: You know like everything, collision detection and response.

        entity->dp.x += entity->ddp.x * dt;
        entity->dp.y += entity->ddp.y * dt;

        entity->p.x += entity->dp.x * dt;
        entity->p.y += entity->dp.y * dt;

        // shitty clamp
        entity->p.x = MAX(entity->p.x, 0);
        entity->p.y = MAX(entity->p.y, 0);
        entity->p.x = MIN(entity->p.x, WORLD_WIDTH);
        entity->p.y = MIN(entity->p.y, WORLD_HEIGHT);

        entity->ddp.x = 0;
        entity->ddp.y = 0;
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
    SimulationState prev = create_new_one_player_game();
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