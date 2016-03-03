#include <stdint.h>
#include <stdbool.h>
#include "core.h"
#include "map_vars.h"

typedef struct {
    uint8_t id;
    bool delete;

    int16_t def;
    uint16_t cd, cooldown;
    uint8_t level;

    union {
        uint8_t value[2];
        struct {
            uint8_t info, cd;
        };
    } update;

    abilityvar_t;
} ability_t;

typedef struct {
    uint8_t id;
    bool delete;

    int16_t def;
    uint16_t lifetime, duration;
    uint8_t level;
    uint16_t proxy;

    union {
        uint8_t value[2];
        struct {
            uint8_t info, lifetime;
        };
    } update;

    statevar_t;
} state_t;

typedef struct {
    bool delete;

    int16_t def;
    uint16_t proxy;
    uint8_t owner, team, level, unused;
    uint32_t vision;

    /* position */
    point pos;
    uint16_t angle, new_angle;

    /* animation */
    anim_t anim;
    bool anim_forced, anim_set, anim_cont;

    /* hp / mana / damage */
    uint32_t hp, mana;
    uint32_t maxhp, maxmana;
    uint32_t damage, damage_max, heal;
    uint16_t damage_source;

    /* ability / state */
    uint8_t nability, nability_new;
    ability_t ability[16];
    uint8_t ability_freeid[16], ability_freetime[16], ability_history[256];

    uint8_t nstate, nstate_new;
    state_t state[64];

    uint8_t state_freeid[64], state_freetime[64], state_history[256];
    //state_t *state;

    union {
        uint8_t value[8];
        struct {
            uint8_t spawn, info, pos, anim, hp, mana, ability, state;
        };
    } update;

    /* */
    point teleport, wt;
    uint32_t wt_range;
    delta push;
    bool tp, walk, turn;
    bool refresh;

    uint8_t norder;
    order_t order[16];

    double movespeed, movespeed_factor;

    entityvar_t;
} entity_t;

uint8_t* ent_full(const entity_t *ent, uint16_t id, uint8_t *p);
uint8_t* ent_frame(const entity_t *ent, uint16_t id, uint8_t *p, uint8_t frame, uint8_t df);
uint8_t* ent_remove(const entity_t *ent, uint16_t id, uint8_t *p);
