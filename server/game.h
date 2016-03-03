#include "core.h"
#include "defs.h"
#include "main.h"

enum {
    anim_idle,
    anim_bored,
    anim_walk,
    anim_death,
    anim_spawn,
    anim_attack,
    anim_disabled,
    anim_taunt,
    anim_spell0,
    anim_spell1,
    anim_spell2,
    anim_spell3,
    anim_spell4,
    anim_spell5,
    anim_spell6,
    anim_spell7,
};

enum {
    permanent = 0xFFFF,
    continuous = 0,
};

#define areaofeffect(g, _pos, distance, _group, func) \
{ \
    uint32_t __dist = (distance); \
    array_for(&g->ent, target) { \
        if ((def(target)->group & (_group)) == 0) continue; \
        if (!inrange(_pos,target->pos,__dist)) continue; \
        func \
    } \
}

#define globaleffect(g, _group, func) \
{ \
    array_for(&g->ent, target) { \
        if ((def(target)->group & (_group)) == 0) continue; \
        func \
    } \
}

#define __super(p, f, ...) p##_##f(__VA_ARGS__)
#define _super(p, f, ...) __super(p, f, __VA_ARGS__)
#define super(...) _super(_parent, _function, __VA_ARGS__)
#define rgba(r,g,b,a) ((r) | ((g) << 8) | ((b) << 16) | ((a) << 24))

#define max_coord (map_size * 16 * 65536 - 1)
#define ent_none 0xFFFF

#define tooltip_none {-1, -1}
#define tooltip(x) {text_##x##_name, .desc = text_##x##_desc}
#define stooltip(x) {text_##x##_name, .desc = text_##x##_state}

#define def(x) _Generic((x), \
                        entity_t* : (&entitysdef[(x)->def]), \
                        const entity_t* : (&entitysdef[(x)->def]), \
                        ability_t* : (&abilitysdef[(x)->def]), \
                        const ability_t* : (&abilitysdef[(x)->def]), \
                        state_t* : (&statesdef[(x)->def]), \
                        const state_t* : (&statesdef[(x)->def]))
//#define def(x) (&entitydef[(x)->def])

/* entity */
entity_t *spawnunit2(game_t *g, uint16_t def_id, point pos, uint16_t angle, uint8_t owner, uint8_t team);
entity_t *spawnunit1(game_t *g, uint16_t def_id, entity_t *source);
entity_t *get_ent(game_t *g, uint16_t ent_id);
void delete(entity_t *ent);

void teleport(entity_t *ent, point p);
void push(entity_t *ent, vec v);
void setfacing(game_t *g, entity_t *ent, uint16_t angle);
void setproxy(game_t *g, entity_t *ent, entity_t *proxy);

void setanim(const game_t *g, entity_t *ent, uint8_t id, uint16_t len, bool restart, bool forced);
void damage(game_t *g, entity_t *ent, entity_t *source, uint32_t amount, uint8_t type);
void heal(game_t *g, entity_t *ent, uint32_t amount);
void givemana(game_t *g, entity_t *ent, uint32_t amount);
void refresh(game_t *g, entity_t *ent);

bool status(entity_t *ent, uint16_t status);

/* ability */
ability_t *giveability(game_t *g, entity_t *ent, uint16_t def_id);
void changeability(game_t *g, entity_t *ent, ability_t *a, uint16_t def_id);
void removeabilities(entity_t *ent);
ability_t *getability(entity_t *ent, uint16_t def_id);
ability_t *getability_id(entity_t *ent, uint8_t id);

void setcooldown(game_t *g, entity_t *ent, ability_t *a, uint16_t cooldown);
void setcd(game_t *g, entity_t *ent, ability_t *a, uint16_t cd);
void startcooldown(game_t *g, entity_t *ent, ability_t *a);

/* state */
state_t* applystate(game_t *g, entity_t *ent, uint16_t proxy, uint16_t def_id, uint8_t level,
                    uint16_t duration, uint16_t immunity_flags);
state_t* applystate_stack(game_t *g, entity_t *ent, uint16_t proxy, uint16_t def_id, uint8_t level,
                    uint16_t duration, uint16_t immunity_flags);

void expirestate(entity_t *ent, uint16_t def_id);
void expire_id(entity_t *ent, uint8_t id);
void expire_all(entity_t *ent);

void aura(game_t *g, entity_t *ent, uint16_t state, uint8_t level, uint32_t radius, uint8_t group);

/* effect */
void playeffect(game_t *g, entity_t *ent, uint16_t effect, uint16_t proxy, uint16_t duration);

/* teams */
bool isally(game_t *g, entity_t *ent, entity_t *target);

/* random */
bool chance(double a);
uint32_t randint(uint32_t max);

/* player */
void movecam(game_t *g, uint8_t id, point p);
void setgold(game_t *g, uint8_t id, uint16_t amount);
void msgplayer(game_t *g, uint8_t id, const char *msg, int len);
void msgall(game_t *g, const char *msg, int len);
#define msgall_str(g, msg) msgall(g, msg, sizeof(msg) - 1)

/* map */
bool tile_isopen(game_t *g, uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void tile_open(game_t *g, uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void tile_close(game_t *g, uint8_t x, uint8_t y, uint8_t w, uint8_t h);

#include "functions.h"

