#include <stdint.h>
#include <stdbool.h>
#include <net.h>
#include <ip.h>
#include <array.h>
#include "event.h"
#include "entity.h"
#include "protocol.h"
#include "map.h"

enum {
    cl_none = 0,
    cl_disconnected = 1,
    cl_downloading = 2,
    cl_loading = 3,

    cl_loaded = 4,
    cl_loaded_delta = 5,
    cl_loaded_reset = 6,
};

#define name_size 16
#define cl_timeout 200

#define maxpart ((compressed_size - 1) / part_size)

/* hacky macros */
#define for_ability(ent, name) for (ability_t *name, *__i = 0; \
    name = &ent->ability[(size_t) __i], (size_t) __i < ent->nability; __i = (void*)((size_t) __i + 1))
#define for_ability_new(ent, name) for (ability_t *name, *__i = 0; \
    name = &ent->ability[(size_t) __i], (size_t) __i < ent->nability_new; __i = (void*)((size_t) __i + 1))

#define cl_id(g, p) array_id(&((g)->cl), p)
#define ent_id(g, p) array_id(&((g)->ent), p)

typedef struct {
    bool active;
    uint8_t firstseen, lastseen, unused;
} centity_t;

typedef struct {
    uint16_t key;
    uint8_t status, timeout, seq, flags, frame, rate;
    int16_t part;
    addr_t addr; //storage in addr padding?

    point cam_pos;

    uint16_t lost[255];
    uint8_t nlost, losti;

    playerdata_t player;

    centity_t ent[65535];
    uint16_t ents[65535];
    uint16_t nent;

    event_t ev;
} client_t;

typedef struct {
    uint32_t key;
    uint16_t value;
    uint8_t players, maxplayers;
    uint64_t checksum;
    uint8_t name[16], desc[32];
} gameinfo_t;

typedef struct {
    int sock;
    uint32_t frame;
    bool update_map;

    addr_t master_addr;
    iptable_t table;
    gameinfo_t info;
    map_t map;

    struct {
        uint16_t key;
        uint8_t fps, id;
        uint32_t size, inflated;
    } conn_rep;

    array(client_t, uint8_t, 255) cl;
    array(entity_t, uint16_t, 65535) ent;

    uint8_t data[compressed_size];
} game_t;

/* map*/
void global_init(game_t *g);
void global_frame(game_t *g);

void player_join(game_t *g, playerdata_t *p);
bool player_leave(game_t *g, playerdata_t *p);
void player_frame(game_t *g, playerdata_t *p);

