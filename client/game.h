#include <stdint.h>
#include <stdbool.h>
#include <net.h>
#include <array_client.h>
#include "math.h"
#include "view.h"
#include "text.h"
#include "map.h"
#include "particle.h"
#include "audio.h"
#include "gfx.h"
#include "bind.h"
#include "conn.h"
#include "../server/defstruct.h"
#include "../server/protocol.h"

typedef struct {
    bool start;
    int var[4];
} effect_t;

typedef struct {
} shop_t;

typedef struct {
    int16_t def;
    uint16_t cd, cooldown;
} ability_t;

typedef struct {
    int16_t def;
    uint16_t lifetime, duration;

    effect_t effect;
} state_t;

typedef struct {
    int16_t def;
    uint16_t proxy;
    uint8_t team, owner, level;

    uint32_t x, y;
    uint16_t angle;

    uint16_t anim_frame;
    uint8_t anim, anim_len;

    uint16_t hp, maxhp, mana, maxmana;

    uint8_t modifiers;

    vec3 pos;

    effect_t effect;
    int voice_sound;

    vec4 modcolor;

    uint8_t nability, aid[16], nstate, sid[64];
    ability_t ability[16];
    state_t state[64];
} entity_t;

typedef struct {
    playerdata_t d;
    int8_t status, unused;
    uint16_t gold;
} player_t;

enum {
    entity,
    ability,
    state,
    effect,
};

#define def(g, x) _Generic((x), \
                    entity_t* : &((const entitydef_t*)g->def[entity])[x->def], \
                    const entity_t* : &((const entitydef_t*)g->def[entity])[x->def], \
                    ability_t* : &((const abilitydef_t*)g->def[ability])[x->def], \
                    const ability_t* : &((const abilitydef_t*)g->def[ability])[x->def], \
                    state_t* : &((const statedef_t*)g->def[state])[x->def], \
                    const state_t* : &((const statedef_t*)g->def[state])[x->def])

#define idef(g, x, id) (&((x##def_t*)g->def[x])[id])
#define edef(g, x, id) (&((x##def_t*)g->def[effect + effect_##x])[id])

typedef struct {
    gfx_t gfx;
    view_t view;
    map_t map;
    psystem_t particle;
    audio_t audio;
    bindlist_t bind;

    double pan_edge, pan_mouse, pan_key;

    uint64_t time; /* timestamp */
    double smooth; /* frame interpolation value [0,1[ */

    ivec2 mouse_pos;    /* mouse coordinates (only valid if mouse_in=1) */
    bool mouse_locked;  /* mouse currently locked to the window? */
    bool mouse_in;      /* mouse inside the window? */

    bool middle, select; //

    ivec2 sel_start, sel_end;   /* selection screen coordinates */
    uint16_t sel_ent[255];      /* entities in selection */
    uint16_t target_ent;        /* entity under mouse */
    point target_pos;           /* game coordinates of mouse */
    uint8_t nsel;               /* number of entities in selection */
    uint8_t target;             /* current targetting flags */
    uint8_t sel_first; //

    point click;            /* position of last ground order */
    uint64_t click_time;    /* time of last ground order */
    uint64_t voice_time;    /* time of last unit voice response */

    float zoom, theta;    /* view parameters */
    vec2 cam_pos;

    uint8_t fps;
    bool loaded;

    void* model[max_models];
    font_t font[num_font];

    int width, height;

    conn_t conn;

    uint32_t key, size, inflated;
    uint8_t *data;
    uint32_t parts_left;
    bool sent_lost;

    /* ui */
    bool minimap_down;
    int8_t slot_down;

    uint8_t action_target, action;
    int8_t action_queue;
    uint8_t action_alt;

    int8_t binding;
    uint16_t bind_id, map_id;
    uint8_t bind_modifier;

    uint8_t keys[256];
    unsigned num_keys;

    /* defs */
    // : boundschecks?
    void* def[num_defs];
    char *text;

    array(entity_t, uint16_t, 65535) ent;
    array(player_t, uint8_t, 255) player;

    /* tmp buffer (used for: texture loading, ..)*/
    uint8_t __attribute__ ((aligned (16))) tmp[256 * 256 * 4]; //tmp[4096 * 24];
} game_t;

//#define entity_loop(g, ent, i) i = 0; while (ent = &g->entity[g->id[i]], ++i <= g->nentity)
#define state_loop(ent, s, i) i = 0; while (s = &ent->state[ent->sid[i]], ++i <= ent->nstate)
#define ability_loop(ent, a, i) i = 0; while (a = &ent->ability[ent->aid[i]], ++i <= ent->nability)

void game_netframe(game_t *g);
void game_netorder(game_t *g, uint8_t order, uint8_t target, int8_t queue, uint8_t alt);
bool game_netinit(game_t *g);

vert2d_t* gameui(game_t *g, vert2d_t *v);

bool game_ui_click(game_t *g, int button);
bool ui_move(game_t *g, int dx, int dy);
void ui_buttonup(game_t *g, int button);
bool ui_wheel(game_t *g, double delta);

void gamelist_refresh(game_t *g);

void game_directconnect(game_t *g, const addr_t *addr);
void game_connect(game_t *g, const addr_t *addr, uint32_t key);
void game_disconnect(game_t *g);

void loadmap(game_t *g);

void state_clear(game_t *g, entity_t *ent, state_t *s);

vec3 entity_bonepos(game_t *g, entity_t *ent, int8_t bone, vec3 v);
void entity_netframe(game_t *g, entity_t *ent, uint8_t delta);
void entity_clear(game_t *g, entity_t *ent);

void gameui_reset(game_t *g);

enum {
    cursor_arrow,
    cursor_text,
    cursor_target,
};

void quit(game_t *g);
void setcursor(game_t *g, uint8_t cursor);
bool lockcursor(game_t *g, bool lock);
bool thread(void (func)(void*), void *arg);
void audio_thread(void *args);

void game_mouseover(game_t *g);

/* */
void game_frame(game_t *g);
bool game_init(game_t *g, unsigned w, unsigned h, unsigned argc, char *argv[]);
void game_resize(game_t *g, int w, int h);
void game_exit(game_t *g);

void game_keydown(game_t *g, uint8_t key);
void game_keyup(game_t *g, uint8_t key);
void game_keysym(game_t *g, uint32_t sym);
void game_char(game_t *g, char *ch, int size);

void game_motion(game_t *g, int x, int y);
void game_button(game_t *g, int x, int y, int button);
void game_buttonup(game_t *g, int button);
void game_wheel(game_t *g, double delta);

