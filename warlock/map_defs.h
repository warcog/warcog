typedef struct {
    uint16_t attack_cooldown, attack_time;
    uint32_t attack_damage, attack_range;
} entitymdef_t;

typedef struct {
    struct {
        uint16_t base, perlevel;
    } cooldown;
} abilitymdef_t;

#define cooldown_params(x,y) {msec(x), msec(y)}

typedef struct {
    uint16_t status;
} statemdef_t;
