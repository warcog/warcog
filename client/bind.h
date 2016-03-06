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

typedef struct {
    union {
        struct { uint16_t map, ability; };
        uint32_t map_ability;
    };
    struct {
        uint8_t type, trigger, num_key;
        uint8_t keys[9];
    };
} bind_t;

typedef struct {
    unsigned count, extra_count;
    bind_t *extra;
    uint16_t list[32768 + num_builtin];
    bind_t data[32768 + num_builtin];
} bindlist_t;

void bind_init(bindlist_t *b, const abilitydef_t *def, unsigned count, unsigned map);
void bind_set(bindlist_t *b, unsigned id, bind_t data);
void bind_save(bindlist_t *b);

bool bind_match(bind_t b, unsigned key, uint8_t *keys, unsigned num_key);
