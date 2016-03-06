#include "game.h"
#include "model.h"
#include "keycodes.h"
#include <stdio.h>

//TODO: dont play sounds at feet

static uint16_t seffect(const game_t *g, const state_t *s)
{
    return (s->def < 0) ? (s->def & 0x7fff) : def(g, s)->effect;
}

static void effect_netframe(game_t *g, entity_t *ent, effect_t *eff, int16_t id, uint8_t delta)
{
    int i;
    const effectsub_t *d;
    const particledef_t *pdef;
    const sounddef_t *sdef;
    const mdlmoddef_t *mdef;
    vec3 pos;

    if (id < 0)
        return;

    d = idef(g, effect, id)->sub;

    for (i = 0; i < 4; i++, d++) {
        pdef = edef(g, particle, d->id);
        sdef = edef(g, sound, d->id);
        mdef = edef(g, mdlmod, d->id);

        switch (d->type) {
        case effect_particle:
            if (!eff->start)
                eff->var[i] = pdef->rate;

            if (eff->var[i] >= delta) {
                eff->var[i] -= delta;
                break;
            }

            eff->var[i] = pdef->rate;

            pos = entity_bonepos(g, ent, pdef->bone, vec3(pdef->x, pdef->y, pdef->z));
            //pos = add(pos, vec3(pdef->x, pdef->y, pdef->z));
            //pos = add(scale(pos, 65536.0), vec3(0.5, 0.5, 0.5));
            particle_new(&g->particle, pos, d->id, pdef->lifetime);
            break;
        case effect_sound:
            if (!eff->start)
                eff->var[i] = audio_play(&g->audio, sdef->sound, ent->pos, sdef->loop, sdef->volume);
            else if (eff->var[i] >= 0)
                if (audio_move(&g->audio, eff->var[i], ent->pos))
                    eff->var[i] = -1;
            break;
        case effect_mdlmod:
            ent->modcolor = colorvec(mdef->color_start);
            break;
        }
    }

    eff->start = 1;
}

static void effect_clear(game_t *g, entity_t *ent, effect_t *eff, int16_t id)
{
    (void) ent;

    int i;
    const effectsub_t *d;

    if (id < 0)
        return;

    eff->start = 0;
    d = idef(g, effect, id)->sub;
    for (i = 0; i < 4; i++, d++) {
        switch (d->type) {
        case effect_sound:
            if (eff->var[i] >= 0)
                audio_stop(&g->audio, eff->var[i]);
            break;
        }
    }
}

vec3 entity_bonepos(game_t *g, entity_t *ent, int8_t bone, vec3 v)
{
    vec3 res;

    if (def(g, ent)->model < 0)
        return ent->pos;

    res = model_transform(g->model[def(g, ent)->model], ent->anim & 127,
                         (double) ent->anim_frame / ent->anim_len, bone, v);

    res.xy = rot2(res.xy, theta(ent->angle));
    res = add(ent->pos, scale(res, def(g, ent)->modelscale));
    /* TODO: rotation */
    return res;
}

void entity_netframe(game_t *g, entity_t *ent, uint8_t delta)
{
    int i;
    ability_t *a;
    state_t *s;

    /* animation */
    if (ent->anim_len && !(ent->anim & 128))
        ent->anim_frame = (ent->anim_frame + delta) % ent->anim_len;

    /* ability cooldown */
    ability_loop(ent, a, i)
        a->cd = (a->cd <= delta) ? 0 : (a->cd - delta);

    ent->modcolor = vec4(1.0, 1.0, 1.0, 1.0);

    /* state duration / effect */
    state_loop(ent, s, i) {
        s->lifetime = (s->lifetime <= delta) ? 0 : (s->lifetime - delta);
        effect_netframe(g, ent, &s->effect, seffect(g, s), delta);
    }

    effect_netframe(g, ent, &ent->effect, def(g, ent)->effect, delta);

    /* voice sound
    if (ent->voicesound >= 0)
        if (audio_move(&audio, ent->voicesound, ent->pos))
            ent->voicesound = -1; */

}

void entity_frame(game_t *g, entity_t *ent, double delta)
{
    //delta is time since last netframe
    //TODO: smooth anim, etc

    (void) g;
    (void) ent;
    (void) delta;
}

void state_clear(game_t *g, entity_t *ent, state_t *s)
{
    effect_clear(g, ent, &s->effect, seffect(g, s));
    s->def = -1;
}

void entity_clear(game_t *g, entity_t *ent)
{
    int i;
    state_t *s;
    ability_t *a;

    state_loop(ent, s, i)
        state_clear(g, ent, s);

    ability_loop(ent, a, i)
        a->def = -1;

    effect_clear(g, ent, &ent->effect, def(g, ent)->effect);

    if (ent->voice_sound >= 0)
        audio_stop(&g->audio, ent->voice_sound);

    ent->def = -1;
    ent->voice_sound = -1;
    ent->nability = 0;
    ent->nstate = 0;

    //memset(ent, 0, sizeof(*ent));
}

struct arg {
    game_t *g;
    vec3 pos, ray, ray_inv;
    float d;
};

static bool intersect_tiles(void *user, vec3 min, vec3 max, treesearch_t *t)
{
    struct arg *arg = user;
    float f;
    vec3 res;

    f = ray_intersect_box(arg->pos, arg->ray_inv, min, max);
    if (f == INFINITY)
        return 0;
    if (t->depth != arg->g->map.size_shift)
        return 1;

    res = map_intersect_tile(&arg->g->map, arg->pos, arg->ray, t->x, t->y);
    if (res.z < arg->d) {
        arg->d = res.z;
        arg->g->target |= tf_ground;
        arg->g->target_pos = point(res.x * 65536.0, res.y * 65536.0);
        //TODO exit search?
    }
    return 0;
}

void game_mouseover(game_t *g)
{
    const entitydef_t *def;
    vec3 pos, ray, ray_inv;
    vec3 min, max;
    float f, d;
    struct arg arg;

    if (!g->loaded)
        return;

    g->target = 0;

    pos = g->view.ref.pos;
    ray = view_ray(&g->view, g->mouse_pos);
    ray_inv = inv(ray);

    d = INFINITY;
    array_for(&g->ent, ent) {
        def = def(g, ent);

        if (!(def->uiflags & ui_target))
            continue;

        min = sub(ent->pos, vec3(def->boundsradius, def->boundsradius, 0.0));
        max = add(ent->pos, vec3(def->boundsradius, def->boundsradius, def->boundsheight));

        f = ray_intersect_box(pos, ray_inv, min, max);
        if (f < d) {
            d = f;
            g->target = tf_entity;
            g->target_ent = array_id(&g->ent, ent);
        }
    }

    arg = (struct arg){g, pos, ray, ray_inv, INFINITY};
    map_quad_iterate(&g->map, &arg, intersect_tiles);
}


