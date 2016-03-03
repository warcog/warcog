#ifndef MODEL_H
#define MODEL_H

#include <stdint.h>
#include <util.h>
#include "math.h"
#include "gfx.h"

#define maxbones 126

/*
model data format:
note: models originally had additional interpolation data

typedef struct {
    uint32_t frames;
} anim_t;

typedef struct {
    int8_t parent;
    uint8_t ntr, nrt, nsc;
    vec3f pivot;
    uint8_t tr_id[nanim];
    uint8_t rt_id[nanim];
    uint8_t sc_id[nanim];
    tr_t tr[keyframes[0]];
    rt_t rt[keyframes[1]];
    sc_t sc[keyframes[2]];
} bone_t;

typedef struct  {
    uint16_t index, texture;
    uint8_t naf, unused[3];
    uint8_t af_id[nanim];
    sc_t af[naf];
} chunk_t;

typedef struct {
	uint32_t frame;
	vec3f vec;
} tr_t;

typedef struct {
	uint32_t frame;
	vec4f vec;
} rt_t;

typedef struct {
    uint32_t frame;
    float value;
} sc_t;

*/

typedef struct {
    uint8_t nchunk, unused, nbone, nanim;
    //anim_t anim[nanim];
    //bone_t bone[nbone];
    //chunk_t chunk[nchunk];
    //uint16_t indices[chunk[nchunk-1].index]
    //vert_t vert[max(indices) + 1];
} model_t;

void model_draw(gfx_t *g, const void *mdl, int anim_id, double frame, vec4 teamcolor, vec4 color,
                uint16_t tex_over);

vec3 model_transform(const void *mdl, int anim_id, double frame, int8_t bone, vec3 v);

bool model_load(data_t p, uint16_t loadtex(void*, uint16_t),
                bool load(void*, const void*, size_t, const uint16_t*, size_t, const void*, size_t),
                void *user) use_result;

#endif
