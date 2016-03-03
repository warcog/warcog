#include "defstruct.h"
#include "map_defs.h"

typedef struct {
    entitydef_t;

    uint8_t group, control;

    animset_t anim[16];
    uint16_t idle_time, bored_time, walk_time;

    uint16_t turnrate;
    double movespeed;
    uint32_t hp, mana;

    uint32_t vision;

    entitymdef_t;
} entitysdef_t;

typedef struct {
    abilitydef_t;

    uint8_t anim;
    uint16_t anim_time, cast_time;
    uint32_t range, range_max;

    abilitymdef_t;
} abilitysdef_t;

typedef struct {
    statedef_t;
    statemdef_t;
} statesdef_t;

extern const mapdef_t mapdef;

extern const entitydef_t entitydef[];
extern const abilitydef_t abilitydef[];
extern const statedef_t statedef[];

extern const entitysdef_t entitysdef[];
extern const abilitysdef_t abilitysdef[];
extern const statesdef_t statesdef[];

/* not used by server */
extern const effectdef_t effectdef[];
extern const particledef_t particledef[];
extern const groundspritedef_t groundspritedef[];
extern const attachmentdef_t attachmentdef[];
extern const mdlmoddef_t mdlmoddef[];
extern const stripdef_t stripdef[];
extern const sounddef_t sounddef[];

extern const char text[];
