#include "entity.h"
#include <string.h>
#include "protocol.h"
#include "util.h"

static uint8_t* write(uint8_t *p, const void *data, int len)
{
    memcpy(p, data, len);
    return p + len;
}

static uint8_t history_max(const uint8_t *history, uint8_t frame, uint8_t df)
{
    uint8_t res;

    res = history[frame];
    while (frame++ != df)
        if (res < history[frame])
            res = history[frame];

    return res;
}

uint8_t* ent_full(const entity_t *ent, uint16_t id, uint8_t *p)
{
    int i;
    const ability_t *a;
    const state_t *s;

    p = write16(p, id);
    *p++ = 0xFF;

    p = write16(p, ent->def);
    p = write16(p, ent->proxy);
    *p++ = ent->owner;
    *p++ = ent->team;
    *p++ = ent->level;
    //p = write32(p, ent->vision);

    p = write(p, &ent->pos, sizeof(ent->pos));
    p = write16(p, ent->angle);

    p = write(p, &ent->anim, sizeof(ent->anim));

    p = write16(p, ent->hp >> 16);
    p = write16(p, ent->maxhp >> 16);

    p = write16(p, ent->mana >> 16);
    p = write16(p, ent->maxmana >> 16);

    for (i = 0; i < ent->nability; i++) {
        a = &ent->ability[i];

        *p++ = a->id;
        p = write16(p, a->def);
        p = write16(p, a->cooldown);
        p = write16(p, a->cd);
    }
    *p++ = 0xFF;

    for (i = 0; i < ent->nstate; i++) {
        s = &ent->state[i];

        *p++ = s->id;
        p = write16(p, s->def);
        p = write16(p, s->duration);
        p = write16(p, s->lifetime);
    }
    *p++ = 0xFF;

    return p;
}

static uint8_t* ability_frame(const ability_t *a, uint8_t *p, uint8_t frame, uint8_t dif)
{
    uint8_t val, *flags, *start;

    start = p;
    *p++ = a->id;
    flags = p++;
    *flags = 0;

    if (val = frame - a->update.info, dif >= val) {
        *flags |= flag_info;
        p = write16(p, a->def);
        p = write16(p, a->cooldown);
    }

    if (val = frame - a->update.cd, dif >= val) {
        *flags |= flag_cd;
        p = write16(p, a->cd);
    }

    return (*flags) ? p : start;
}

static uint8_t* state_frame(const state_t *s, uint8_t *p, uint8_t frame, uint8_t dif)
{
    uint8_t val, *flags, *start;

    start = p;
    *p++ = s->id;
    flags = p++;
    *flags = 0;

    if (val = frame - s->update.info, dif >= val) {
        *flags |= flag_info;
        p = write16(p, s->def);
        p = write16(p, s->duration);
    }

    if (val = frame - s->update.lifetime, dif >= val) {
        *flags |= flag_lifetime;
        p = write16(p, s->lifetime);
    }

    return (*flags) ? p : start;
}

uint8_t* ent_frame(const entity_t *ent, uint16_t id, uint8_t *p, uint8_t frame, uint8_t df)
{
    uint8_t dif, val, *flags, *start;
    int i, j, k;

    dif = frame - df;

    start = p;
    p = write16(p, id);
    flags = p++;
    *flags = 0;

    if (val = frame - ent->update.info, dif >= val) {
        *flags |= flag_info;
        p = write16(p, ent->def);
        p = write16(p, ent->proxy);
        *p++ = ent->owner;
        *p++ = ent->team;
        *p++ = ent->level;
        //p = write32(p, ent->vision);
    }

    if (val = frame - ent->update.pos, dif >= val) {
        *flags |= flag_pos;
        p = write(p, &ent->pos, sizeof(ent->pos));
        p = write16(p, ent->angle);
    }

    if (val = frame - ent->update.anim, dif >= val) {
        *flags |= flag_anim;
        p = write(p, &ent->anim, sizeof(ent->anim));
    }

    if (val = frame - ent->update.hp, dif >= val) {
        *flags |= flag_hp;
        p = write16(p, ent->hp >> 16);
        p = write16(p, ent->maxhp >> 16);
    }

    if (val = frame - ent->update.mana, dif >= val) {
        *flags |= flag_mana;
        p = write16(p, ent->mana >> 16);
        p = write16(p, ent->maxmana >> 16);
    }

    if (val = frame - ent->update.ability, dif >= val) {
        *flags |= flag_ability;

        for (i = 0; i < ent->nability; i++)
            p = ability_frame(&ent->ability[i], p, frame, dif);

        for (j = history_max(ent->ability_history, frame, df); i < j; i++) {
            k = ent->ability_freeid[i];
            if (val = frame - ent->ability_freetime[k], dif >= val) {
                *p++ = k;
                *p++= 0;
            }
        }

        *p++ = 0xFF;
    }

    if (val = frame - ent->update.state, dif >= val) {
        *flags |= flag_state;

        for (i = 0; i < ent->nstate; i++)
            p = state_frame(&ent->state[i], p, frame, dif);

        for (j = history_max(ent->state_history, frame, df); i < j; i++) {
            k = ent->state_freeid[i];
            if (val = frame - ent->state_freetime[k], dif >= val) {
                *p++ = k;
                *p++= 0;
            }
        }

        *p++ = 0xFF;
    }

    return (*flags) ? p : start;
}

uint8_t* ent_remove(const entity_t *ent, uint16_t id, uint8_t *p)
{
    (void) ent;

    p = write16(p, id);
    *p++ = 0;

    return p;
}
