//#include <game.h>

#include "game.h"

#include <math.h>
#include <stdio.h>

#include "gen.h"
#include "text.h"

#define __super(p, f, ...) p##_##f(__VA_ARGS__)
#define _super(p, f, ...) __super(p, f, __VA_ARGS__)
#define super(...) _super(_parent, _function, __VA_ARGS__)

//#include "text.h"
#include "textures.h"
#include "models.h"
#include "sounds.h"

#define map_name "Warlock"

#ifndef TOURNAMENT
#define map_desc "FFA practice mode"
#else
#define map_desc "Tournament mode"
#endif

#define map_tileset tex_ashen_grass, tex_ashen_dirt
//#define map_tileset tex_cave_lava, tex_cave_dirt
#define map_iconset tex_buttons, tex_icon_holdposition, tex_icon_stop, tex_icon_move

enum {
    armor_types = 2,
};

/* status */
enum {
    /* disables */
    silenced = 1,
    perplexed = 2,
    disarmed = 4,
    immobilized = 8,
    _cantturn = 16,
    stunned = silenced | perplexed | disarmed | immobilized | _cantturn,
    restrained = 32,

    //
    dead = 64,

    /* vision */
    invisible = 256,
    revealed = 512,
    truesight = 1024,

    /* immunity */
    immune_magic = 4096,
    immune_physical = 8192,
    invulnerable = immune_magic | immune_physical,
};

/* group flags */
enum {
    group_hero = 1,
    group_nonhero = 2,
    group_unit = 3, /* hero and non hero */
    group_building = 4,
    group_combat = 7, /* unit and building */
    group_projectile = 8,
    group_gadget = 16,
};

#define hero_radius distance(6)

/* damage types */
#define damage_attack_flag 1
enum {
    damage_physical_spell,
    damage_physical_attack,
    damage_magic_spell,
    damage_magic_attack,
    damage_pure_spell,
    damage_pure_attack,
};

entity_t* spawnprojectile(game_t *g, uint16_t def, entity_t *src, point p,
                          double dist, double speed, double angle);

void applypush(game_t *g, entity_t *ent, point p, point source, double amount);
bool ignore(entity_t *ent, entity_t *target);


