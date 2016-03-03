#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

enum {
    msg_bits = 3,
    ctl_in = 4,
    ctl_out = 8,
    ctl_missing = 16, /* (only for msg_data) */
    ctl_notconn = 128,
};

enum {
    flag_info = 1,
    flag_pos = 2,
    flag_anim = 4,
    flag_hp = 8,
    flag_mana = 16,
    flag_ability = 32,
    flag_state = 64,
    flag_spawn = 128,

    /* ability */
    flag_cd = 2,

    /* state */
    flag_lifetime = 2,
};

/* server->client */
enum {
    msg_frame,
    msg_delta,
    msg_reset,
    msg_data,
};

enum {
    disconnected,
    timeout,
    kicked,
};

enum {
    ev_join,
    ev_zzz,

    /* _removed must be odd */
    ev_disconnect,
    ev_disconnect_removed,
    ev_timeout,
    ev_timeout_removed,
    ev_kicked,
    ev_kicked_removed,

    ev_servermsg,
    ev_setgold,
    ev_movecam,
    ev_chat,
    ev_end = 0xFF,
};

/* client->server */
enum {
    msg_setrate,
    msg_lost,
    msg_loaded,
    msg_chat,
    msg_order,
    msg_disconnect,
};

#define max_rate 10
#define part_size 1408

typedef struct {
    uint8_t id, team, name[16], color[3];
    int8_t unused;
    uint16_t hero;
} playerdata_t;

#endif
