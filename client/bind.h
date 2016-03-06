#include <stdint.h>
#include <stdbool.h>
#include "../server/defstruct.h"

enum {
    bind_none,
    bind_ability,
    bind_builtin,
    bind_slot,
};

enum {
    builtin_move,
    builtin_stop,
    builtin_hold,
    num_builtin,
};

enum {
    modifier_shift,
};

typedef struct {
    union {
        struct { uint16_t map, ability; };
        uint32_t map_ability;
    };
    struct {
        uint8_t type, trigger, num_key, alt;
        uint8_t keys[8];
    };
} bind_t;

typedef struct {
    uint8_t alt, mod;
} bindmatch_t;

typedef struct {
    unsigned count, extra_count;
    bind_t *extra;
    uint8_t mod, modifier[7];
    uint16_t list[32768 + num_builtin];
    bind_t data[32768 + num_builtin];
} bindlist_t;

void bind_init(bindlist_t *b, const abilitydef_t *def, unsigned count, unsigned map);
void bind_set(bindlist_t *b, unsigned id, bind_t data);
void bind_save(bindlist_t *b);

uint8_t bind_ismodifier(bindlist_t *b, uint8_t key);
void bind_update_modifiers(bindlist_t *b, uint8_t *keys, unsigned num_key);

unsigned bind_match(bindlist_t *b, unsigned id, uint8_t key, uint8_t *keys, unsigned num_key);
