#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <util.h>

static uint8_t* readability(entity_t *ent, uint8_t *data, int *r_len, uint8_t val)
{
    ability_t *a;
    uint8_t id, flags;
    int len, i, j;

    len = *r_len;

    do {
        if (--len < 0)
            return 0;

        id = *data++;
        if (id == 0xFF)
            break;

        flags = 0xFF;
        if (!(val & flag_spawn)) {
            if (--len < 0)
                return 0;
            flags = *data++;
        }

        a = &ent->ability[id];
        if (!flags) {
            a->def = -1;
            continue;
        }

        if (a->def < 0) {
            assert(flags & flag_info);
            ent->aid[ent->nability++] = id;
        }

        if (flags & flag_info) {
            if ((len -= 4) < 0)
                return 0;

            memcpy(&a->def, data, 2); data += 2;
            memcpy(&a->cooldown, data, 2); data += 2;

            assert(a->def >= 0);
        }

        if (flags & flag_cd) {
            if ((len -= 2) < 0)
                return 0;

            memcpy(&a->cd, data, 2); data += 2;
        }
    } while (1);

    j = 0;
    ability_loop(ent, a, i)
        if (a->def >= 0)
            ent->aid[j++] = (a - ent->ability);
    ent->nability = j;

    *r_len = len;
    return data;
}

static uint8_t* readstate(game_t *g, entity_t *ent, uint8_t *data, int *r_len, uint8_t val)
{
    state_t *a;
    uint8_t id, flags;
    int len, i, j;

    len = *r_len;

    do {
        if (--len < 0)
            return 0;

        id = *data++;
        if (id == 0xFF)
            break;

        flags = 0xFF;
        if (!(val & flag_spawn)) {
            if (--len < 0)
                return 0;
            flags = *data++;
        }

        a = &ent->state[id];
        if (!flags) {
            state_clear(g, ent, a);
            continue;
        }

        if (a->def == -1) {
            assert(flags & flag_info);
            ent->sid[ent->nstate++] = id;
        }

        if (flags & flag_info) {
            if ((len -= 4) < 0)
                return 0;

            memcpy(&a->def, data, 2); data += 2;
            memcpy(&a->duration, data, 2); data += 2;

            assert(a->def != -1);
        }

        if (flags & flag_lifetime) {
            if ((len -= 2) < 0)
                return 0;

            memcpy(&a->lifetime, data, 2); data += 2;
        }
    } while (1);

    j = 0;
    state_loop(ent, a, i)
        if (a->def != -1)
            ent->sid[j++] = (a - ent->state);
    ent->nstate = j;

    *r_len = len;
    return data;;
}

static int recvgframe(game_t *g, uint8_t msg, uint8_t *data, int len)
{
//TODO CHECK IF EXPECTING RESET
    uint8_t frame, val, delta;
    uint16_t id;
    uint32_t size, i, j;
    point coord;
    player_t *pl;
    entity_t *ent;

    if (--len < 0)
        return 0;

    val = frame = *data++;
    delta = 0;
    if (msg == msg_delta) {
        if (--len < 0)
            return 0;
        val = *data++;
        delta = frame - val;
		printf("delta: %u %u %u %u\n", delta, val, frame, g->conn.frame);
        assert(delta <= 220);
    }

    if (msg != msg_reset && val != g->conn.frame) {
        g->conn.flags ^= ctl_out;
        net_send(g->conn.sock, &g->conn.addr, &g->conn.id, 4);
        return 1;
    }

    g->conn.lastrecv = g->time;

    if (msg == msg_reset) {
        /* player info */
        if (--len < 0)
            return 0;

        val = *data++;
        size = val * sizeof(pl->d);
        if ((len -= size) < 0)
            return 0;

        array_reset(&g->player);
        while (val--) {
            //TODO
            pl = array_get(&g->player, *data);
            array_add(&g->player, pl);

            memcpy(pl, data, sizeof(pl->d)); data += sizeof(pl->d);
            pl->status = 1;
        }

        if ((len -= 8) < 0)
            return 0;

        memcpy(&coord, data, 8); data += 8;
        g->cam_pos = scale(vec2(coord.x, coord.y), 1.0 / 65536.0);

        /* reset entities */
        array_for(&g->ent, ent) {
            entity_clear(g, ent);
            particle_clear(&g->particle);
        }
        array_reset(&g->ent);
    } else {
        /* events */
        do {
            if (--len < 0)
                return 0;

            val = *data++;
            switch (val) {
            case ev_join:
                if ((len -= sizeof(pl->d)) < 0)
                    return 0;

                pl = array_get(&g->player, *data);
                array_add(&g->player, pl);

                memcpy(pl, data, sizeof(pl->d)); data += sizeof(pl->d);
                pl->status = 1; //TODO
                break;

            case ev_disconnect:
            case ev_disconnect_removed:
            case ev_timeout:
            case ev_timeout_removed:
            case ev_kicked:
            case ev_kicked_removed:
                if (--len < 0)
                    return 0;

                pl = array_get(&g->player, *data++);
                if (val & 1)
                    pl->status = 0;
                else
                    pl->status = -1;
                break;

            case ev_servermsg:
                if (--len < 0)
                    return 0;
                val = *data++;

                if ((len -= val) < 0)
                    return 0;

                printf("Server message: %.*s\n", val, data);
                data += val;
                break;

            case ev_movecam:
                if ((len -= 8) < 0)
                    return 0;

                memcpy(&coord, data, 8); data += 8;
                g->cam_pos = scale(vec2(coord.x, coord.y), 1.0 / 65536.0);
                break;
            case ev_setgold:
                if ((len -= 3) < 0)
                    return 0;

                pl = array_get(&g->player, *data++);

                memcpy(&pl->gold, data, 2); data += 2;
                break;
            case ev_chat:

            default:
                assert(0);
            case ev_end:
                break;
            }
        } while (val != ev_end);

        /* entity frame */
        array_for(&g->ent, ent)
            entity_netframe(g, ent, delta + 1);

        particle_netframe(&g->particle, delta + 1);
    }

    /* entities */
    while (len) {
        if ((len -= 3) < 0)
            return 0;

        memcpy(&id, data, 2); data += 2;
        val = *data++;

        ent = array_get(&g->ent, id);

        if (!val) {
            entity_clear(g, ent);
            continue;
        }

        assert(((val & flag_spawn) != 0) == (ent->def < 0));

        if (ent->def < 0)
            array_add(&g->ent, ent);

        if (val & flag_info) {
            if ((len -= 7) < 0)
                return 0;

            memcpy(&ent->def, data, 2); data += 2;
            memcpy(&ent->proxy, data, 2); data += 2;

            ent->owner = *data++;
            ent->team = *data++;
            ent->level = *data++;
        }

        if (val & flag_pos) {
            if ((len -= 10) < 0)
                return 0;

            memcpy(&ent->x, data, 4); data += 4;
            memcpy(&ent->y, data, 4); data += 4;
            memcpy(&ent->angle, data, 2); data += 2;
        }

        if (val & flag_anim) {
            if ((len -= 4) < 0)
                return 0;

            memcpy(&ent->anim_frame, data, 2); data += 2;
            ent->anim = *data++;
            ent->anim_len = *data++;
        }

        if (val & flag_hp) {
            if ((len -= 4) < 0)
                return 0;

            memcpy(&ent->hp, data, 2); data += 2;
            memcpy(&ent->maxhp, data, 2); data += 2;
        }

        if (val & flag_mana) {
            if ((len -= 4) < 0)
                return 0;

            memcpy(&ent->mana, data, 2); data += 2;
            memcpy(&ent->maxmana, data, 2); data += 2;
        }

        if (val & flag_ability) {
            data = readability(ent, data, &len, val);
            if (!data)
                return 0;
        }

        if (val & flag_state) {
            data = readstate(g, ent, data, &len, val);
            if (!data)
                return 0;
        }
    }

    /* update entity and player id list */
    j = 0;
    array_for(&g->ent, ent) {
        if (ent->def < 0)
            continue;

        ent->pos = vec3((float) ent->x / 65536.0, (float) ent->y / 65536.0,
                        map_height(&g->map, ent->x, ent->y));

        if (ent->voice_sound >= 0)
            if (audio_move(&g->audio, ent->voice_sound, ent->pos))
                ent->voice_sound = -1;

        j = array_keep(&g->ent, ent, j);
    }
    array_update(&g->ent, j);

    j = 0;
    array_for(&g->player, pl) {
        if (pl->status < 0)
            continue;

        j = array_keep(&g->player, pl, j);
    }
    array_update(&g->player, j);;

    /* validate selection */
    for (j = 0, i = 0; i < g->nsel; i++)
        if (array_get(&g->ent, g->sel_ent[i])->def >= 0)
            g->sel_ent[j++] = g->sel_ent[i];
    g->nsel = j;

    g->conn.frame = frame + 1;
    return 1;
}

static void recvframe(game_t *g, uint8_t *data, int len)
{
    int i, j;
    uint8_t seq, flags, *p;
    uint16_t part_id;
    uint8_t buf[2 * 256];

    if ((len -= 2) < 0)
        return;

    seq = *data++;
    flags = *data++;

    if (flags & ctl_notconn)
        return;

    g->conn.seq_c = seq;

    /* send lost messages */
    if ((flags & ctl_in) != (g->conn.flags & ctl_in)) {
        g->conn.flags ^= ctl_in;
        conn_resend(&g->conn);
    }

    if ((flags & ctl_out) != (g->conn.flags & ctl_out))
        return;

    if ((flags & msg_bits) == msg_data) {
        g->conn.lastrecv = g->time;

        if (!g->parts_left)
            return;

        if (len == 2) {
            p = buf;
            *p++ = msg_lost;
            *p++ = j = g->parts_left > 255 ? 255 : g->parts_left;

            for (i = 0; j; i++)
                if (!g->data[g->size + i])
                    p = write16(p, i), j--;

            conn_send(&g->conn, buf, p - buf);
        }

        if ((len -= 2) < 0)
            return;

        memcpy(&part_id, data, 2); data += 2;

        if (!len)
            return;

        assert(part_id <= (g->size - 1) / part_size && len <= part_size);
        assert(len == part_size || part_id * part_size == g->size - len);

        if (!g->data[g->size + part_id]) {
            g->data[g->size + part_id] = 1;
            memcpy(g->data + part_id * part_size, data, len);

            if (!--g->parts_left) {
                printf("download complete\n");
                loadmap(g);

                buf[0] = msg_lost;
                buf[1] = 0;
                buf[2] = msg_loaded;
                conn_send(&g->conn, buf, 3);
            }
        }

        return;
    }

    recvgframe(g, flags & msg_bits, data, len);
    //assert();
}

void game_directconnect(game_t *g, const addr_t *addr)
{
    conn_connect(&g->conn, addr, 0, g->time);
}

void game_connect(game_t *g, const addr_t *addr, uint32_t key)
{
    conn_connect(&g->conn, addr, key, g->time);
}

void game_disconnect(game_t *g)
{
    if (g->loaded)
        bind_save(&g->bind);

    g->loaded = 0;
    free(g->data);

    conn_disconnect(&g->conn);
}

void game_netframe(game_t *g)
{
    addr_t addr;
    int len;
    void *data;
    uint32_t parts;

    union {
        struct {
            uint32_t key;
        };
        struct {
            uint16_t ckey;
            uint8_t fps, id;
            uint32_t size, inflated;
        };
        uint8_t data[65536];
    } buf;

    while ((len = net_recv(g->conn.sock, &addr, &buf, sizeof(buf))) >= 0) {
        if (addr.cmp != g->conn.addr.cmp || !g->conn.active)
            continue;

        if (buf.key & 0x8000) {
            if (g->conn.connected)
                continue;

            if (len == 64 && !g->conn.key) {
                g->conn.key = buf.key;
                send_connect(&g->conn, g->time);
            }

            if (len != 12 || (buf.ckey & 0x7FFF) != g->conn.ckey ||
                buf.inflated >= 0x1000000 || !buf.size || buf.size >= 0x800000)
                continue;

            parts = (buf.size - 1) / part_size + 1;
            data = malloc(buf.size + parts);
            if (!data)
                continue;

            memset(data + buf.size, 0, parts);

            g->conn.id = buf.id;
            g->conn.seq_out = 0;
            g->conn.frame = 0;
            g->conn.flags = 0;

            g->fps = buf.fps;
            g->conn.seq_c = 0;
            g->conn.connected = 1;
            g->conn.lastrecv = g->conn.timer = g->time;

            g->size = buf.size;
            g->inflated = buf.inflated;
            g->parts_left = parts;
            g->data = data;

            printf("connected\n");
            continue;
        }

        if ((len -= 2) >= 0 && g->conn.connected && buf.ckey == g->conn.ckey)
            recvframe(g, buf.data + 2, len);
    }

    if (!conn_frame(&g->conn, g->time))
        game_disconnect(g);
}

void game_netorder(game_t *g, uint8_t order, uint8_t target, int8_t queue, uint8_t alt)
{
    struct {
        uint8_t msg;
        uint8_t count;
        uint8_t queue;
        uint8_t order;
        uint8_t target_type;
        uint8_t alt;
        union {
            struct {
                uint8_t target_pos[8];
                uint16_t ent0[255];
            };
            struct {
                uint16_t target_ent;
                uint16_t ent1[255];
            };
            struct {
                uint16_t ent2[255];
            };
        };
    } msg;

    uint16_t *ent;
    unsigned size;

    msg.msg = msg_order;
    msg.count = g->nsel;
    msg.queue = queue | ((g->bind.mod & 1) ? 128 : 0);
    msg.order = order;
    msg.target_type = target;
    msg.alt = alt;

    if (target == target_pos) {
        memcpy(msg.target_pos, &g->target_pos, 8);
        ent = msg.ent0;
    } else if (target == target_ent) {
        msg.target_ent = g->target_ent;
        ent = msg.ent1;
    } else {
        ent = msg.ent2;
    }

    memcpy(ent, g->sel_ent, g->nsel * 2);
    size = (uint8_t*) ent - &msg.msg + g->nsel * 2;

    conn_send(&g->conn, &msg, size);
}

bool game_netinit(game_t *g)
{
    return conn_init(&g->conn);
}

//TODO entity replacement
