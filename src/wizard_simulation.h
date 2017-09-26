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

// we'll start with zelda because that's a lot easier honestly!
// so, input will also include an attack button!
// we'll have a "pressed attach" or "attacked" flag or something.
// we'll keep track of character facing (left right up down)
// Dunno if you favor left right or up down when traveling and how that affects combat.
// I also want a WIZARD BLINK for dodging.

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

V2 v2_add(V2 a, V2 b)
{
    return (V2){
        .x = a.x + b.x,
        .y = a.y + b.y
    };
}

V2 v2_sub(V2 a, V2 b)
{
    return (V2){
        .x = a.x - b.x,
        .y = a.y - b.y
    };
}

V2 v2_scale(V2 v, float f)
{
    return (V2){
        .x = v.x * f,
        .y = v.y * f
    };
}

// Not data oriented at this point. Gonna probably start with a huge array like this and work it back? Maybe?
// @TODO: Figure out later, just doing this for now cuz it's easier?
typedef enum {
    ET_NONE,
    ET_PLAYER,
    ET_WALL,
    ET_FIREBALL
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
    V2 p;
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
    EF_COLLIDES = 0x1,
    EF_FRICTION = 0x2
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

typedef enum {
    FACING_UP,
    FACING_DOWN,
    FACING_LEFT,
    FACING_RIGHT,
} Facing;

typedef struct _entity {
    uint32_t entity_id;
    uint32_t flags;

    EntityType type;
    Facing facing;
    V2 p;   // position
    V2 dp;  // velocity
    V2 ddp; // acceleration
    Geometry collision_pieces[32]; // collision geometry positions are relative to entity position.
    uint8_t num_collision_pieces;

    struct _entity *next;
} Entity;

// Need my and other players inputs for this frame.
// Need to know the time stuff happens at.

// @TODO: These are a bunch of bools, I can pack them in an int.
// @TODO: Not sure how to do facing yet, it's a thing that changes when different directions are pressed. Gonna make it part of input for now.
typedef struct {
    int pressing_up;
    int pressing_down;
    int pressing_left;
    int pressing_right;

    int pressed_attack;
    int pressed_blink;

    Facing facing; // last direction pressed
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
        .flags = EF_COLLIDES | EF_FRICTION,
        .type = ET_PLAYER,
        .facing = FACING_RIGHT,
        .p = {.x=0, .y=0},
        .dp = {.x=0, .y=0},
        .ddp = {.x=0, .y=0},
        .collision_pieces = {
            {.type = GT_AABB,
            .aabb = {
                .p = {.x=0,.y=0},
                .min = {.x=-0.3, .y=0},
                .max = {.x=0.4, .y=1}}},
            0
        },
        .num_collision_pieces = 1,
        .next = NULL
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

// @HARDCODE
#define SPEED 40
#define FRICTION 10
#define WORLD_WIDTH 15
#define WORLD_HEIGHT 15

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

            // @TODO
            player_entity->facing = input->facing;
            // @TODO
            if (input->pressed_attack) {
                // @Q: Is this something that should happen before or after moving entities?
                // Prolly wanna like batch up entity spawns and do them after the simulation step as they're gonna depend on
                // iteractions and stuff too.

                Entity *fireball = NULL;
                if (next->first_free_entity) {
                    fireball = next->first_free_entity;
                    next->first_free_entity = fireball->next;
                } else if (next->num_entities < LEN(next->entities)) {
                    fireball = next->entities + next->num_entities++;
                } else {
                    assert(0); // Error: Can't allocate new entity. TODO
                }

                if (fireball) {
                    // some thoughts on fireballs
                    // only add player v in directons perpendicular to the facing direction
                    // that way they can shoot up and down but running doesn't make bullets faster.
                    V2 fireball_direction = {0};
                    V2 fireball_dp = {0};
                    const int fireball_speed = 10;
                    switch(player_entity->facing) {
                        case FACING_LEFT: {
                            fireball_direction = (V2){.x=-1, .y=0};
                            fireball_dp = (V2){.x=fireball_direction.x * fireball_speed, .y=player_entity->dp.y};
                            break;
                        }
                        case FACING_RIGHT: {
                            fireball_direction = (V2){.x=1, .y=0};
                            fireball_dp = (V2){.x=fireball_direction.x * fireball_speed, .y=player_entity->dp.y};
                            break;
                        }
                        case FACING_UP: {
                            fireball_direction = (V2){.x=0, .y=1};
                            fireball_dp = (V2){.x=player_entity->dp.x, .y=fireball_direction.y * fireball_speed};
                            break;
                        }
                        case FACING_DOWN: {
                            fireball_direction = (V2){.x=0, .y=-1};
                            fireball_dp = (V2){.x=player_entity->dp.x, .y=fireball_direction.y * fireball_speed};
                            break;
                        }
                    };

                    fireball->entity_id = 2; // @TODO
                    fireball->flags = EF_COLLIDES;
                    fireball->type = ET_FIREBALL;
                    fireball->facing = player_entity->facing;
                    fireball->p = (V2){.x=player_entity->p.x, .y=player_entity->p.y+1};
                    fireball->dp = fireball_dp;
                    fireball->ddp = (V2){.x=0, .y=0};
                    fireball->collision_pieces[0] = (Geometry){
                        .type = GT_CIRCLE,
                        .circle = {
                            .p = {.x=0, .y=0},
                            .r = 10
                        }
                    };
                    fireball->num_collision_pieces = 1;
                    fireball->next = NULL;
                }
            }

            frame_acceleration.x *= SPEED;
            frame_acceleration.y *= SPEED;
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

        // @TODO: ODE for real friction.
        // @TODO: Does this seem really wrong?
        //printf("ddp before friction: %f", entity->ddp.x);
        if (flags_is_set(entity->flags, EF_FRICTION)) {
            entity->ddp.x += -FRICTION * entity->dp.x;
            entity->ddp.y += -FRICTION * entity->dp.y;
        }
        //printf(", ddp after friction: %f\n", entity->ddp.x);

        // @TODO: You know like everything, collision detection and response.

        V2 next_dp = {0};
        next_dp.x = (entity->ddp.x * dt) + entity->dp.x;
        next_dp.y = (entity->ddp.y * dt) + entity->dp.y;

        V2 next_p = {0};
        next_p.x = (0.5 * entity->ddp.x * dt*dt) + (entity->dp.x * dt) + entity->p.x;
        next_p.y = (0.5 * entity->ddp.y * dt*dt) + (entity->dp.y * dt) + entity->p.y;

        entity->dp = next_dp;
        entity->p = next_p;

        // shitty clamp to world
        entity->p.x = MAX(entity->p.x, 0);
        entity->p.y = MAX(entity->p.y, 0);
        entity->p.x = MIN(entity->p.x, WORLD_WIDTH);
        entity->p.y = MIN(entity->p.y, WORLD_HEIGHT);

        // clear frame acceleration
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