#include <stdint.h>
#include <stdbool.h>
#include "../server/defstruct.h"

enum {
    bind_type = 0xC0,
    bind_ability = 0x00,
    bind_builtin = 0x40,
    bind_slot = 0x80,
    bind_unknown = 0xC0
};

typedef struct {
    union {
        struct { uint16_t map, ability; };
        uint32_t map_ability;
    };
    union {
        struct { uint8_t info; uint8_t keys[3]; };
        uint32_t key_int;
    };
} bind_t;

typedef struct {
    unsigned count, extra_count;
    bind_t *extra;
    uint16_t list[32768 + 16];
    bind_t data[32768 + 16];
} bindlist_t;

//#define bind(map, ability, keyint) ((bind_t){{{map, ability}},{.key_int = keyint}})
void bind_init(bindlist_t *b, const abilitydef_t *def, unsigned count, unsigned map);
void bind_set(bindlist_t *b, unsigned id, bind_t data);
void bind_save(bindlist_t *b);

bool bind_match(bind_t b, unsigned key, uint8_t *keys, unsigned num_key);
