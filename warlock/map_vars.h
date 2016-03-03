typedef struct {
    uint16_t ent;
    //uint8_t state;
    uint8_t sid;
    double var[2];
    bool bvar[1];
} abilityvar_t;

typedef struct {
    uint8_t aid;
    uint16_t tvar[1];
    vec vec[1];
    point pos[1];
} statevar_t;

typedef struct {
    uint16_t source, target;

    uint16_t status;

    double armor[2];

    uint32_t attack_range;
    uint16_t attack_time;

    double bonus_damage, attack_speed;

    double var[2];
    vec vec[2];
    uint16_t tvar[1];
} entityvar_t;
