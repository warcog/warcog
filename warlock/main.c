#include "main.h"
#include <stdio.h>

static uint8_t /*arena, size, */state;
static uint16_t timer;

static struct {
    uint16_t len, id[65535];
} list;

#define list_start() list.len = 0
#define list_add(g, p) list.id[list.len++] = array_id(&g->ent, p)
#define list(g, name, func) { \
    int i; entity_t *name; \
    for (i = 0; i < list.len; i++) { \
        name = array_get(&g->ent, list.id[i]); \
        func; \
    } \
}

enum {
    gstate_wait,
    gstate_start,
    gstate_shop,
    gstate_main,
};

/*entity_t* spawnbuilding(game_t *g, uint16_t def, entity_t *src, point p, uint8_t w, uint8_t h)
{
    uint8_t x, y;

    x = totile(p.x + (tile(1) - 1) - tile(w) / 2);
    y = totile(p.y + (tile(1) - 1) - tile(h) / 2);

    if (!tile_isopen(g, x, y, w, h))
        return 0;

    tile_close(g, x, y, w, h);

    p.x = tile(x) + tile(w) / 2;
    p.y = tile(y) + tile(h) / 2;
    return spawnunit2(g, def, p, 0, src->owner, src->team);
}*/

entity_t* spawnprojectile(game_t *g, uint16_t def, entity_t *src, point p,
                          double dist, double speed, double angle)
{
    entity_t *ent;
    vec v, d;

    v = rotate(vec_normp(src->pos, p), angle);
    d = scale(src->vec[1], src->var[1]);

    ent = spawnunit1(g, def, src);

    ent->var[0] = 0.0;
    ent->vec[0] = add(scale(v, speed), d);
    ent->tvar[0] = msec(2000);

    ent->pos = addvec(src->pos, add(scale(v, dist), ent->vec[0]));

    setproxy(g, ent, src);

    return ent;
}

void applypush(game_t *g, entity_t *ent, point p, point source, double amount)
{
    vec v;
    double d, v1, v2;

    d = 1.0 + ent->mana / mana(100.0);
    givemana(g, ent, amount * 3.0 + 0.5);

    v1 = ent->var[0];
    v2 = amount * d;
    v = add(scale(ent->vec[0], v1 * v1),
            scale(vec_normp(source, p), v2 * v2));
    d = mag(v);

    ent->vec[0] = scale(v, 1.0 / d);
    ent->var[0] = sqrt(d);
}

bool ignore(entity_t *ent, entity_t *target)
{
    (void) ent;
    (void) target;

    return 0;
}

void global_init(game_t *g)
{
    (void) g;
    //arena = paint_new(g, tiles(16, 16), tiles(48, 48), 1, 1);
    //size = 16;

    state = gstate_wait;
    timer = 0;
}

void global_frame(game_t *g)
{
    char msg[128];
    int count, gold;
    uint16_t angle;

    switch (state) {
    case gstate_wait:
        if (++timer == sec(10)) {
            timer = 0;
            msgall_str(g, "Game will start when at least 2 players are ready.");
        }

        count = 0;
        globaleffect(g, group_hero,
            if (getability(target, ability_unready))
                count++;
        );

        if (count < 2)
            break;

        msgall_str(g, "Game will start in 10 seconds.");
        msgall_str(g, "Set your status to ready to play the next round.");

        state = gstate_start;
        timer = 0;
        break;
    case gstate_start:
        if (++timer != sec(10))
            break;
        timer = 0;

        list_start();
        globaleffect(g, group_hero,
            if (getability(target, ability_unready))
                list_add(g, target);
        );

        if (list.len < 2) {
            msgall_str(g, "Less than 2 players ready: round cancelled.");
            state = gstate_wait;
            break;
        }

        gold = 30 + randint(30);
        list(g, ent,
            setgold(g, ent->owner, gold);
            expirestate(ent, state_dead);
            applystate(g, ent, ent_none, state_shop, 0, permanent, 0);
            removeabilities(ent);
        );

        msgall(g, msg, sprintf(msg, "%u gold this round. Round starts in 20 seconds!", gold));
        state = gstate_shop;
        break;
    case gstate_shop:
        if (++timer != sec(20))
            break;
        timer = 0;

        list(g, ent,
            expire_all(ent);

            angle = (double) full_circle * i / list.len;
            setfacing(g, ent, (int) (angle + 0.5) - half_circle);
            teleport(ent, addvec(tiles(32, 32), vec_angle(angle, distance(128))));

            refresh(g, ent);
            ent->var[0] = 0.0;
        );

        state = gstate_main;
        break;
    case gstate_main:
        count = 0;
        list(g, ent,
            if (!status(ent, dead))
                count++;
        );

        if (count >= 2)
            break;

        //winner
        list(g, ent,
            applystate(g, ent, ent_none, state_dead, 0, permanent, 0);
        );

        timer = 0;
        state = gstate_wait;
        break;
    }
}

void player_join(game_t *g, playerdata_t *p)
{
    p->team = p->id;

    spawnunit2(g, warlock, tiles(32, 34), 0, p->id, p->team);

    damage(g, spawnunit2(g, warlock, tiles(32, 32), 0, p->id, p->team), 0, hp(10), 0);
    //p->hero = ent_id(g, ent);

    printf("join %s (%u)\n", p->name, p->id);

    movecam(g, p->id, tiles(32, 32));
}

bool player_leave(game_t *g, playerdata_t *p)
{
    printf("leave %s (%u)\n", p->name, p->id);

    array_for(&g->ent, ent)
        if (ent->owner == p->id)
            ent->delete = 1;

    return 1;
}

void player_frame(game_t *g, playerdata_t *p)
{
    (void) g;
    (void) p;
}
