#include "model.h"

#include <string.h>

typedef struct {
    uint32_t frames;
} anim_t;

typedef struct {
    int8_t parent;
    uint8_t ntr, nrt, nsc;
    vec3 pivot;
} bone_t;

typedef struct {
    uint16_t index, texture;
    uint8_t naf, unused[3];
} chunk_t;

typedef struct {
    uint32_t frame;
    float value;
} sc_t;

typedef struct {
	uint32_t frame;
	vec3 vec;
} tr_t;

typedef struct {
	uint32_t frame;
	vec4 vec;
} rt_t;

static float get_alpha(const uint8_t *p, const uint8_t *tr, int anim_id, double frame)
{
    uint8_t start, end;
    sc_t *k;

    start = (!anim_id) ? 0 : tr[anim_id - 1];
    end = tr[anim_id];
    k = (sc_t*) p; k += start;

    if (start == end)
        return 1.0;

    if (start + 1 == end || frame <= k->frame)
        return k->value;

    while (++start != end && (k + 1)->frame <= frame) k++;

    //no interpolation
    return k->value;
}

static void get_trans(vec3 *res, const uint8_t *p, const uint8_t *tr, int anim_id, double frame)
{
    uint8_t start, end;
    tr_t *k, *j;
    float t;

    start = (!anim_id) ? 0 : tr[anim_id - 1];
    end = tr[anim_id];
    k = (tr_t*) p; k += start;

    if(start == end) {
        memset(res, 0, sizeof(*res));
        return;
    }

    if(start + 1 == end || frame <= k->frame) {
        *res = k->vec;
        return;
    }

    while ((++k)->frame < frame);

    j = k - 1;
    t = (float)(frame - j->frame) / (float)(k->frame - j->frame);
    *res = lerp(j->vec, k->vec, t);
}

static void get_rot(vec4 *res, const uint8_t *p, const uint8_t *qt, int anim_id, double frame)
{
    uint8_t start, end;
    rt_t *k, *j;
    float t;

    start = (!anim_id) ? 0 : qt[anim_id - 1];
    end = qt[anim_id];
    k = (rt_t*) p; k += start;

    if(start == end) {
        *res = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    if(start + 1 == end || frame <= k->frame) {
        *res = k->vec;
        return;
    }

    while ((++k)->frame < frame);

    j = k - 1;
    t = (float)(frame - j->frame) / (float)(k->frame - j->frame);
    *res = nlerp(j->vec, k->vec, t);
}

static float get_scale(const uint8_t *p, const uint8_t *sc, int anim_id, double frame)
{
    uint8_t start, end;
    sc_t *k, *j;
    float t;

    start = (!anim_id) ? 0 : sc[anim_id - 1];
    end = sc[anim_id];
    k = (sc_t*) p; k += start;

    if(start == end)
        return 1.0;

    if(start + 1 == end || frame <= k->frame)
        return k->value;

    while ((++k)->frame < frame);

    j = k - 1;
    t = (float)(frame - j->frame) / (float)(k->frame - j->frame);
    return (j->value * (1.0 - t) + k->value * t);
}

#define align4(p) (void*)(((size_t)(p) + 3) & (~3))

void model_draw(gfx_t *g, const void *mdl, int anim_id, double frame, vec4 teamcolor, vec4 color,
                uint16_t tex_over)
{
    /* assumes valid model data */
    uint8_t ngeo, nbone, nanim;
    uint8_t *p, *tr, *qt, *sc;

    chunk_t *ck;
    anim_t *anim;
    bone_t *b;

    float scale;
    vec3 trans;
    vec4 ptrans, rot, prot;

    int i, offset;
    float alpha;
    uint16_t tex, texture;

    /* */
    vec4 rtrans[maxbones];
    vec4 rotation[maxbones];

    /* parse header */
    p = (uint8_t*) mdl;
    ngeo = p[0];
    nbone = p[2];
    nanim = p[3];

    /*  */
    if (anim_id >= nanim)
        anim_id = 0, frame = 0.0;

    anim = (anim_t*) &p[4];
    p = (uint8_t*) &anim[nanim];
    anim += anim_id;

    frame = frame * anim->frames;

    /* calculate transformations for frame */
    i = 0;
    do {
        b = (bone_t*)p;
        tr = (uint8_t*)&b[1];
        qt = tr + nanim;
        sc = qt + nanim;
        p = sc + nanim;

        p = align4(p);

        get_trans(&trans, p, tr, anim_id, frame);
        p += b->ntr * sizeof(tr_t);

        get_rot(&rot, p, qt, anim_id, frame);
        p += b->nrt * sizeof(rt_t);

        scale = get_scale(p, sc, anim_id, frame);
        p += b->nsc * sizeof(sc_t);

        if(b->parent < 0) {
            ptrans = prot = vec4(0.0, 0.0, 0.0, 1.0);
        } else {
            ptrans = rtrans[b->parent];
            prot = rotation[b->parent];
        }

        rotation[i] = qmul(rot, prot);
        rtrans[i].w = ptrans.w * scale;

        trans = add(ptrans.xyz, scale(qrot(add(b->pivot, trans), prot), ptrans.w));
        rtrans[i].xyz = sub(trans, scale(qrot(b->pivot, rotation[i]), rtrans[i].w));
    } while (++i < nbone);

    /* load transformations */
    gfx_mdl_params(g, rotation, rtrans, nbone, color);

    /*  */
    offset = 0;
    tex = 0xFFFF;
    for (i = 0; i < ngeo; i++) {
        ck = (chunk_t*) p;
        tr = p + sizeof(*ck);
        p = tr + nanim;
        p = align4(p);

        alpha = get_alpha(p, tr, anim_id, frame);
        p += ck->naf * sizeof(sc_t);

        if (alpha != 0.0) {
            texture = tex_over == 0xFFFF ? ck->texture : tex_over;
            if (tex != texture)
                gfx_mdl_texture(g, tex = texture, teamcolor);
            gfx_mdl_draw(g, offset, ck->index);
        }
        offset = ck->index;
    }
}

/* todo: implement this correctly */
vec3 model_transform(const void *mdl, int anim_id, double frame, int8_t bone, vec3 v)
{
    uint8_t nbone, nanim;
    uint8_t *p, *tr, *qt, *sc;

    anim_t *anim;
    bone_t *b;
    float scale;
    vec3 trans;
    vec4 ptrans, rot, prot;
    int i;

    /* */
    vec4 rtrans[maxbones];
    vec4 rotation[maxbones];

    if (bone < 0)
        return vec3(0.0, 0.0, 0.0);

    /* parse header */
    p = (uint8_t*) mdl;
    //ngeo = p[0];
    nbone = p[2];
    nanim = p[3];

    anim = (anim_t*)&p[4];
    p = (uint8_t*) &anim[nanim];
    anim += anim_id;

    frame = frame * anim->frames;

    /* calculate transformations for frame */
    i = 0;
    do {
        b = (bone_t*) p;
        tr = (uint8_t*) &b[1];
        qt = tr + nanim;
        sc = qt + nanim;
        p = sc + nanim;

        p = align4(p);

        get_trans(&trans, p, tr, anim_id, frame);
        p += b->ntr * sizeof(tr_t);

        get_rot(&rot, p, qt, anim_id, frame);
        p += b->nrt * sizeof(rt_t);

        scale = get_scale(p, sc, anim_id, frame);
        p += b->nsc * sizeof(sc_t);

        if(b->parent < 0) {
            ptrans = prot = vec4(0.0, 0.0, 0.0, 1.0);
        } else {
            ptrans = rtrans[b->parent];
            prot = rotation[b->parent];
        }

        rotation[i] = qmul(rot, prot);
        rtrans[i].w = ptrans.w * scale;

        trans = add(ptrans.xyz, scale(qrot(add(b->pivot, trans), prot), ptrans.w));
        rtrans[i].xyz = sub(trans, scale(qrot(b->pivot, rotation[i]), rtrans[i].w));

        if (i == bone)
            return add(trans, qrot(scale(v, rtrans[i].w), rotation[i]));
        //return add(qrot3(scale(add(v, trans), rtrans[i].w), rotation[i]), rtrans[i].xyz);

    } while(++i < nbone);

    return vec3(0.0, 0.0, 0.0);
}

bool model_load(data_t p, unsigned loadtex(void*, unsigned),
                bool load(void*, const void*, size_t, const uint16_t*, size_t, const void*, size_t),
                void *user)
{
    size_t i, mdl_size, vert_max;
    model_t *mdl;
    anim_t *anim;
    bone_t *bone;
    chunk_t *chunk;
    uint16_t *index;
    void *vert;

    uint8_t *tr_id, *rt_id, *sc_id, *af_id;
    tr_t *tr;
    rt_t *rt;
    sc_t *sc;
    sc_t *af;

    mdl = dp_read(&p, sizeof(model_t));
    if (!mdl)
        return 0;

    if (!mdl->nchunk || !mdl->nanim || !mdl->nbone || mdl->nbone > maxbones)
        return 0;

#define f(x) (((x) + 3) & ~3)
    if (!dp_expect(&p,
                   mdl->nanim * sizeof(anim_t) +
                   mdl->nbone * (sizeof(bone_t) + f(mdl->nanim * 3)) +
                   mdl->nchunk * (sizeof(chunk_t) + f(mdl->nanim))))
        return 0;
#undef f

    anim = dp_next(&p, mdl->nanim * sizeof(anim_t));
    /* TODO valid anims */
    (void) anim;

    for (i = 0; i < mdl->nbone; i++) {
        bone = dp_next(&p, sizeof(bone_t));

        if (!dp_expect(&p,
                       bone->ntr * sizeof(tr_t) +
                       bone->nrt * sizeof(rt_t) +
                       bone->nsc * sizeof(sc_t)))
            return 0;

        tr_id = dp_next(&p, mdl->nanim);
        rt_id = dp_next(&p, mdl->nanim);
        sc_id = dp_next(&p, mdl->nanim);
        dp_next(&p, -(mdl->nanim * 3) & 3);

        tr = dp_next(&p, bone->ntr * sizeof(tr_t));
        rt = dp_next(&p, bone->nrt * sizeof(rt_t));
        sc = dp_next(&p, bone->nsc * sizeof(sc_t));

        /* TODO valid keyframes */
        (void) tr_id;
        (void) tr;
        (void) rt_id;
        (void) rt;
        (void) sc_id;
        (void) sc;
    }

    for (i = 0; i < mdl->nchunk; i++) {
        chunk = dp_next(&p, sizeof(chunk_t));

        if (!dp_expect(&p, chunk->naf * sizeof(sc_t)))
            return 0;

        af_id = dp_next(&p, mdl->nanim);
        dp_next(&p, -mdl->nanim & 3);

        af = dp_next(&p, chunk->naf * sizeof(sc_t));

        /* TODO valid alphaframes */
        (void) af_id;
        (void) af;

        chunk->texture = loadtex(user, chunk->texture);
    }
    mdl_size = p.data - (uint8_t*) mdl;

    index = dp_read(&p, ((chunk->index + 1) & ~1) * sizeof(uint16_t));
    if (!index)
        return 0;

    /* validate indices and get vertex count */
    vert_max = 0;
    for (i = 0; i < chunk->index; i++)
        vert_max = index[i] >= vert_max ? index[i] + 1u : vert_max;

    vert = dp_read(&p, vert_max * vert_size);
    if (!vert || !dp_empty(&p))
        return 0;

    if (!load(user, mdl, mdl_size, index, chunk->index, vert, vert_max))
        return 0;

    return 1;
}
