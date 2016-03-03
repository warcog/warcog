#include <stdint.h>

enum {
    num_defs = 10,
    bone_none = -1,
};

typedef struct __attribute__ ((aligned (8))) {
    uint8_t size;

    uint16_t ndef[num_defs];
    uint32_t textlen;
    uint8_t name[16], desc[32];
    int16_t tileset[16], iconset[8];
} mapdef_t;

typedef struct {
    uint8_t count;
    uint16_t sound[7];
} voice_t;

typedef struct {
    uint8_t count, id[3];
} animset_t;

typedef struct {
    int32_t name, desc;
} tooltip_t;

enum {
    ui_control = 1,
    ui_target = 2,
};

typedef struct __attribute__ ((aligned (8))) {
    int16_t model, icon, effect;
    uint8_t uiflags;
    tooltip_t tooltip;
    voice_t voice[4];

    float boundsradius, boundsheight, modelscale;
} entitydef_t;

enum {
    tf_passive = 0,
    tf_none = 1,
    tf_ground = 2,
    tf_entity = 4,
};

enum {
    target_none,
    target_pos,
    target_ent,
};

typedef struct __attribute__ ((aligned (8))) {
    int16_t icon;
    uint8_t front_queue, slot, priority;

    uint8_t target, target_group;

    tooltip_t tooltip;

    uint32_t key; //
} abilitydef_t;

typedef struct  __attribute__ ((aligned (8))) {
    int16_t icon, effect;
    tooltip_t tooltip;
} statedef_t;

enum {
    effect_none,
    effect_particle,
    effect_strip,
    effect_sound,
    effect_groundsprite,
    effect_mdlmod,
    effect_attachment,
    //effect_effect,
};

typedef struct __attribute__ ((aligned (8))) {
    int8_t bone;
    uint8_t rate, start_frame, end_frame;
    uint16_t lifetime;
    float start_size, end_size;

    uint16_t texture;
    uint8_t psize;
    uint32_t color;

    //uint16_t count;
    //, speed;
    float x, y, z;
} particledef_t;

typedef struct __attribute__ ((aligned (8))) {
    uint8_t bone1, bone2;
    uint16_t texture, lifetime;
    float size, speed;
} stripdef_t;

typedef struct __attribute__ ((aligned (8))) {
    uint8_t loop, volume, no_follow;
    uint16_t sound;
} sounddef_t;

typedef struct __attribute__ ((aligned (8))) {
    uint8_t start_height, end_height, start_size, end_size;
    uint32_t color;
    uint8_t bone, texture;
    uint16_t turnrate, lifetime;
} groundspritedef_t;

typedef struct __attribute__ ((aligned (8))) {
    uint16_t texture, len;
    uint32_t color_start, color_end;
} mdlmoddef_t;

typedef struct __attribute__ ((aligned (8))) {
    uint8_t bone, unused;
    uint16_t model;
    uint8_t color[4];
    float size, x, y, z;
} attachmentdef_t;

typedef struct {
    uint8_t type, id;
} effectsub_t;

typedef struct __attribute__ ((aligned (8))) {
    effectsub_t sub[4];
} effectdef_t;
