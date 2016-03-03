#include "game.h"
#include <string.h>
#include <assert.h>

entity_t *spawnunit2(game_t *g, uint16_t def_id, point pos, uint16_t angle, uint8_t owner, uint8_t team)
{
    entity_t *ent;

    ent = array_new2(&g->ent);
    if (!ent)
        return 0;

    memset(ent, 0, sizeof(*ent)); //todo remove this

    ent->def = def_id;
    ent->proxy = 0xFFFF;

    ent->owner = owner;
    ent->team = team;
    ent->level = 0;
    ent->unused = 0;

    ent->pos = pos;
    ent->angle = angle;

    ent->anim_cont = 1;
    ent->anim.frame = 0;
    ent->anim.id = def(ent)->anim[anim_idle].id[0];
    ent->anim.len = def(ent)->idle_time;

    entity_initmod(g, ent);
    entity_finishmod(g, ent);

    ent->hp = ent->maxhp;
    ent->mana = 0;//ent->maxmana;

    ent->damage = ent->damage_max = ent->heal = 0;//
    ent->damage_source = 0xFFFF;

    ent->nability = ent->nstate = 0;

    ent->update.spawn = g->frame;

    int i;
    for (i = 0; i < 16; i++)
        ent->ability_freeid[i] = i;
    for (i = 0; i < 64; i++)
        ent->state_freeid[i] = i;

    entity_onspawn(g, ent);

    return ent;
}

entity_t *spawnunit1(game_t *g, uint16_t def_id, entity_t *source)
{
    entity_t *ent;

    ent = spawnunit2(g, def_id, source->pos, source->angle, source->owner, source->team);
    //ent->modifiers = owner->modifiers;
    return ent;
}

entity_t *get_ent(game_t *g, uint16_t ent_id)
{
    return (ent_id != 0xFFFF) ? array_get(&g->ent, ent_id) : 0;
}

void delete(entity_t *ent)
{
    ent->delete = 1;
}

void teleport(entity_t *ent, point p)
{
    ent->teleport = p;
    ent->tp = 1;
}

void push(entity_t *ent, vec v)
{
    //TODO
    ent->push.x += v.x;
    ent->push.y += v.y;
}

void setfacing(game_t *g, entity_t *ent, uint16_t angle)
{
    ent->angle = angle;
    ent->update.pos = g->frame;
}

void setproxy(game_t *g, entity_t *ent, entity_t *proxy)
{
    (void) g;
    (void) ent;
    (void) proxy;
}

void setanim(const game_t *g, entity_t *ent, uint8_t id, uint16_t len, bool restart, bool forced)
{
    id = def(ent)->anim[id].id[0];

    if (!forced && ent->anim_forced)
        return;

    if (ent->anim.id == id && !restart)
        return;

    if (len > 255)
        len = 255;

    ent->anim_cont = !restart;
    ent->anim_forced = forced;
    ent->anim_set = 1;

    ent->anim.frame = 0;
    ent->anim.id = id; //TODO
    if (len)
        ent->anim.len = len;
    ent->update.anim = g->frame;
}

void damage(game_t *g, entity_t *ent, entity_t *source, uint32_t amount, uint8_t type)
{
    amount = entity_ondamaged(g, ent, source, amount, type);
    ent->damage += amount;
    if (ent->damage_max < amount) {
        ent->damage_max = amount;
        ent->damage_source = source ? ent_id(g, source) : 0xFFFF;
    }
}

void heal(game_t *g, entity_t *ent, uint32_t amount)
{
    (void) g;
    ent->heal += amount;
}

void givemana(game_t *g, entity_t *ent, uint32_t amount)
{
    ent->update.mana = g->frame;
    ent->mana += amount;
    if (ent->mana > ent->maxmana)
        ent->mana = ent->maxmana;
}

void refresh(game_t *g, entity_t *ent)
{
    for_ability(ent, a)
        setcd(g, ent, a, 0);

    ent->refresh = 1;
}

bool status(entity_t *ent, uint16_t status)
{
    return ((ent->status & status) == status);
}

void setcooldown(game_t *g, entity_t *ent, ability_t *a, uint16_t cooldown)
{
    if (a->cooldown == cooldown)
        return;

    a->cooldown = cooldown;
    a->update.info = g->frame;
    ent->update.ability = g->frame;
}

void setcd(game_t *g, entity_t *ent, ability_t *a, uint16_t cd)
{
    a->cd = cd;
    a->update.cd = g->frame;
    ent->update.ability = g->frame;
}

void startcooldown(game_t *g, entity_t *ent, ability_t *a)
{
    setcd(g, ent, a, a->cooldown);
}

ability_t *giveability(game_t *g, entity_t *ent, uint16_t def_id)
{
    ability_t *a;
    uint8_t id;
    int i;

    if (ent->nability_new == 16)
        return 0;

    id = ent->ability_freeid[ent->nability_new];

    a = ent->ability;
    a += ent->nability_new;
    ent->nability_new++;

    memset(a, 0, sizeof(*a));
    a->id = id;
    a->def = def_id;

    for (i = 0; i != sizeof(a->update); i++)
        a->update.value[i] = g->frame;
    ent->update.ability = g->frame;
    return a;
}

void changeability(game_t *g, entity_t *ent, ability_t *a, uint16_t def_id)
{
    a->def = def_id;

    a->update.info = g->frame;
    ent->update.ability = g->frame;
}

void removeabilities(entity_t *ent)
{
    for_ability(ent, a)
        a->delete = 1;
}

ability_t *getability(entity_t *ent, uint16_t def_id)
{
    for_ability(ent, a)
        if (a->def == def_id)
            return a;
    return 0;
}

ability_t *getability_id(entity_t *ent, uint8_t id)
{
    for_ability(ent, a)
        if (a->id == id)
            return a;
    return 0;
}

state_t* applystate(game_t *g, entity_t *ent, uint16_t proxy, uint16_t def_id, uint8_t level,
                    uint16_t duration, uint16_t immunity_flags)
{
    state_t *s;
    uint8_t id, i;

    if (ent->status & immunity_flags)
        return 0;

    if (ent->nstate_new == 64)
        return 0;

    id = ent->state_freeid[ent->nstate_new];

    s = ent->state;
    s += ent->nstate_new;
    ent->nstate_new++;

    memset(s, 0, sizeof(*s));
    s->id = id;

    s->duration = duration;
    s->lifetime = duration + 1 + (duration != 0xFFFF);
    s->level = level;
    s->proxy = proxy;
    s->def = def_id;

    for (i = 0; i < sizeof(s->update); i++)
        s->update.value[i] = g->frame;
    ent->update.state = g->frame;

    return s;
}

/*state_t* applystate2(game_t *g, entity_t *ent, uint16_t source, uint16_t def_id, uint8_t level,
                    uint16_t duration, uint16_t immunity_flags, bool stack)
{
    state_t *s;
    uint8_t id;
    uint16_t lifetime;
    int i;
    bool reapply, reapply2, update;

    lifetime = duration + 1;

    if (ent->status & immunity_flags)
        return 0;

    reapply = reapply2 = 0;
    if (!stack) {
        for (i = 0; i < ent->nstate; i++) {
            s = &ent->state[i];
            if (s->def == def_id) {
                reapply = 1;
                goto init;
            }
        }

        for (; i < ent->nstate_new; i++) {
            s = &ent->state[i];
            if (s->def == def_id) {
                reapply2 = 1;
                goto init;
            }
        }
    }

    assert(ent->nstate_new < 255);

    id = ent->state_freeid[ent->nstate_new];

    s = realloc(ent->state, (ent->nstate_new + 1) * sizeof(*s));
    if (!s) {
        assert(0);
        return 0;
    }

    ent->state = s;
    s += ent->nstate_new;
    ent->nstate_new++;

    memset(s, 0, sizeof(*s));
    s->id = id;
init:
    update = 1;
    if (reapply2) {
        if (level < s->level)
            level = s->level;
        if (lifetime < s->lifetime)
            lifetime = s->lifetime;
    } else if (reapply) {
        lifetime++;

        update = 0;
        if (level > s->level)
            update = 1;
        else
            level = s->level;

        if (s->duration != duration || update)
            s->update.info = g->frame, update = 1;

        if (s->duration) {
            if (lifetime > s->lifetime)
                s->update.lifetime = g->frame, update = 1;
            else
                lifetime = s->lifetime;
        }
    } else {
        for (i = 0; i != sizeof(s->update); i++)
            s->update.value[i] = g->frame;
    }

    if (update)
        ent->update.state = g->frame;

    s->delete = 0;
    s->duration = duration;
    s->lifetime = lifetime;
    s->level = level;
    s->source = source;
    s->def = def_id;

    return s;
}*/

void expirestate(entity_t *ent, uint16_t def_id)
{
    state_t *s;
    int i;

    for (i = 0; i < ent->nstate; i++) {
        s = &ent->state[i];
        if (s->def == def_id)
            s->lifetime = 1;
    }
}

void expire_id(entity_t *ent, uint8_t id)
{
    state_t *s;
    int i;

    for (i = 0; i < ent->nstate; i++) {
        s = &ent->state[i];
        if (s->id == id) {
            s->lifetime = 1;
            return;
        }
    }
    //shouldnt happen
}

void expire_all(entity_t *ent)
{
    int i;

    for (i = 0; i < ent->nstate; i++)
        ent->state[i].lifetime = 1;
}

void aura(game_t *g, entity_t *ent, uint16_t state, uint8_t level, uint32_t radius, uint8_t group)
{
    (void) state;
    (void) level;

    areaofeffect(g, ent->pos, radius, group,
        if (!isally(g, ent, target))
            continue;

        //applystate(g, target, ent_id(g, ent), state, level, continuous, 0, 0);
    );
}

void playeffect(game_t *g, entity_t *ent, uint16_t effect, uint16_t proxy, uint16_t duration)
{
    applystate(g, ent, proxy, 0x8000 + effect, 0, duration, 0);
}

bool isally(game_t *g, entity_t *ent, entity_t *target)
{
    (void) g;
    return (target->team == ent->team);
}

bool chance(double a)
{
    //TODO
    return (rand() % 1024) / 1024.0 > a;
}

uint32_t randint(uint32_t max)
{
    return (rand() % (max + 1));
}

void movecam(game_t *g, uint8_t id, point p)
{
    client_t *cl;
    uint8_t *data;

    cl = array_get(&g->cl, id);
    data = event_write(&cl->ev, g->frame, 1 + sizeof(p));
    if (!data)
        return;

    data[0] = ev_movecam;
    memcpy(data + 1, &p, sizeof(p));

    cl->cam_pos = p;
}

void setgold(game_t *g, uint8_t id, uint16_t amount)
{
    //TOOD write this event for all players that should know
    client_t *cl;
    uint8_t *data;

    cl = array_get(&g->cl, id);
    data = event_write(&cl->ev, g->frame, 4);
    if (!data)
        return;

    data[0] = ev_setgold;
    data[1] = id;
    memcpy(data + 2, &amount, 2);
}

void msgplayer(game_t *g, uint8_t id, const char *msg, int len)
{
    client_t *cl;
    uint8_t *data;

    cl = array_get(&g->cl, id);
    data = event_write(&cl->ev, g->frame, 2 + len);
    if (!data)
        return;

    data[0] = ev_servermsg;
    data[1] = len;
    memcpy(data + 2, msg, len);
}

void msgall(game_t *g, const char *msg, int len)
{
    array_for(&g->cl, cl) {
        if (cl->status < cl_loaded)
            continue;

        msgplayer(g, cl_id(g, cl), msg, len);
    }
}

bool tile_isopen(game_t *g, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    uint8_t i, j;

    for (j = y; j <= y + h - 1; j++)
        for (i = x; i <= x + w - 1; i++)
            if (g->map.path[j][i])
                return 0;

    return 1;
}

void tile_open(game_t *g, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    uint8_t i, j;

    for (j = y; j <= y + h - 1; j++)
        for (i = x; i <= x + w - 1; i++)
            g->map.path[j][i] = 0;

    g->update_map = 1;
}

void tile_close(game_t *g, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    uint8_t i, j;

    for (j = y; j <= y + h - 1; j++)
        for (i = x; i <= x + w - 1; i++)
            g->map.path[j][i] = 1;

    g->update_map = 1;
}
