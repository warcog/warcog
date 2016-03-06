#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <util.h>

static uint8_t* write16(uint8_t *p, uint16_t val)
{
    memcpy(p, &val, 2);
    return p + 2;
}


static void msg_send(game_t *g, const uint8_t *end)
{
    uint8_t seq, *p;
    int len;

    len = end - g->packet.data;

    net_send(g->sock, &g->addr, &g->packet, 4 + len);
    seq = g->packet.seq++;

    assert(!g->msg[seq].data);

    p = g->msg[seq].data = malloc(len);
    g->msg[seq].len = len;

    assert(p);

    memcpy(p, g->packet.data, len);
}

static void conn_sendlost(game_t *g, uint8_t seq)
{
    uint8_t *p, sseq;
    int len;

    p = g->packet.data;
    len = 0;
    sseq = g->packet.seq;
    g->packet.seq = seq;

    while (seq != sseq) {
        assert(g->msg[seq].data);

        if (len + g->msg[seq].len > part_size) {
            msg_send(g, p);

            p = g->packet.data;
            len = 0;
        }

        memcpy(p, g->msg[seq].data, g->msg[seq].len);
        p += g->msg[seq].len;
        len += g->msg[seq].len;

        free(g->msg[seq].data);
        g->msg[seq].data = NULL;
        seq++;
    }

    msg_send(g, p);
}

static void directconnect(game_t *g, const addr_t *addr)
{
    uint32_t data[16];

    if (!g->addr.family)
        g->lastrecv = g->time;

    g->timer = g->time;
    g->addr.cmp = addr->cmp;

    data[0] = ~0u;
    net_send(g->sock, addr, data, sizeof(data));
}

void game_directconnect(game_t *g, const addr_t *addr)
{
    if (g->connected)
        game_disconnect(g);

    directconnect(g, addr);
}

static void send_connect(game_t *g, const addr_t *addr, uint32_t key)
{
    struct {
        uint8_t ones;
        uint8_t key[4];
        uint8_t have_map, id;
        uint8_t ckey[2];
        uint8_t name[16];
    } msg ;

    if (!g->addr.family)
        g->lastrecv = g->time;

    g->timer = g->time;
    g->key = key;
    g->addr.cmp = addr->cmp;

    msg.ones = 0xff;
    memcpy(msg.key, &key, 4);
    strcpy((char*) msg.name, "newbie");
    msg.have_map = 0;
    msg.id = 0xFF;
    memcpy(msg.ckey, &g->ckey, 2);

    net_send(g->sock, addr, &msg, sizeof(msg));
}

void game_connect(game_t *g, const addr_t *addr, uint32_t key)
{
    if (g->connected)
        game_disconnect(g);

	printf("%u %u\n", addr->ip, addr->port);

    send_connect(g, addr, key);
}

void game_disconnect(game_t *g)
{
    int i;

    if (g->loaded)
        bind_save(&g->bind);

    g->addr.family = 0;
    g->key = 0;
    g->connected = 0;
    g->loaded = 0;
    g->ckey = (g->ckey + 1) & 0x7FFF;

    free(g->data);

    for (i = 0; i < 256; i++)
        free(g->msg[i].data), g->msg[i].data = 0;

    printf("disconnected\n");
}

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
		printf("delta: %u %u %u %u\n", delta, val, frame, g->packet.frame);
        assert(delta <= 220);
    }

    if (msg != msg_reset && val != g->packet.frame) {
        g->packet.flags ^= ctl_out;
        net_send(g->sock, &g->addr, &g->packet, 4);
        return 1;
    }

    g->lastrecv = g->time;

    if (msg == msg_reset) {
        /* player info */
        if (--len < 0)
            return 0;

        val = *data++;
        size = val * sizeof(pl->d);
        if ((len -= size) < 0)
            return 0;

        g->nplayer = 0;
        while (val--) {
            pl = &g->player[*data];
            memcpy(pl, data, sizeof(pl->d)); data += sizeof(pl->d);
            pl->status = 1;
            g->pid[g->nplayer++] = pl->d.id;
        }

        if ((len -= 8) < 0)
            return 0;

        memcpy(&coord, data, 8); data += 8;
        g->cam_pos = scale(vec2(coord.x, coord.y), 1.0 / 65536.0);

        /* reset entities */
        for_entity(g, ent) {
            entity_clear(g, ent);
            particle_clear(&g->particle);
        }
        g->nentity = 0;
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

                pl = &g->player[*data];
                memcpy(pl, data, sizeof(pl->d)); data += sizeof(pl->d);
                pl->status = 1; //TODO
                g->pid[g->nplayer++] = pl->d.id;
                break;

            case ev_disconnect:
            case ev_disconnect_removed:
            case ev_timeout:
            case ev_timeout_removed:
            case ev_kicked:
            case ev_kicked_removed:
                if (--len < 0)
                    return 0;

                pl = &g->player[*data++];
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

                pl = &g->player[*data++];
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
        for_entity(g, ent)
            entity_netframe(g, ent, delta + 1);

        particle_netframe(&g->particle, delta + 1);
    }

    /* entities */
    while (len) {
        if ((len -= 3) < 0)
            return 0;

        memcpy(&id, data, 2); data += 2;
        val = *data++;

        ent = &g->entity[id];

        if (!val) {
            entity_clear(g, ent);
            continue;
        }

        assert(((val & flag_spawn) != 0) == (ent->def < 0));

        if (ent->def < 0)
            g->id[g->nentity++] = id;

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
    for_entity(g, ent) {
        if (ent->def < 0)
            continue;

        ent->pos = vec3((float) ent->x / 65536.0, (float) ent->y / 65536.0,
                        map_height(&g->map, ent->x, ent->y));

        if (ent->voice_sound >= 0)
            if (audio_move(&g->audio, ent->voice_sound, ent->pos))
                ent->voice_sound = -1;

        g->id[j++] = (ent - g->entity);
    }
    g->nentity = j;

    j = 0;
    for_player(g, pl) {
        if (pl->status < 0)
            continue;

        g->pid[j++] = (pl - g->player);
    }
    g->nplayer = j;

    /* validate selection */
    for (j = 0, i = 0; i < g->nsel; i++)
        if (g->entity[g->sel_ent[i]].def >= 0)
            g->sel_ent[j++] = g->sel_ent[i];
    g->nsel = j;

    g->packet.frame = frame + 1;
    return 1;
}

static void recvframe(game_t *g, uint8_t *data, int len)
{
    int i, j;
    uint8_t seq, flags, *p;
    uint16_t part_id;

    if ((len -= 2) < 0)
        return;

    seq = *data++;
    flags = *data++;

    if (flags & ctl_notconn)
        return;

    /* free confirmed messages */
    while (g->rseq != seq) {
        assert(g->msg[g->rseq].data);

        free(g->msg[g->rseq].data);
        g->msg[g->rseq].data = NULL;
        g->rseq++;
    }

    /* send lost messages */
    if ((flags & ctl_in) != (g->packet.flags & ctl_in)) {
        g->packet.flags ^= ctl_in;
        conn_sendlost(g, seq);
    }

    if ((flags & ctl_out) != (g->packet.flags & ctl_out))
        return;

    if ((flags & msg_bits) == msg_data) {
        g->lastrecv = g->time;

        if (!g->parts_left)
            return;

        if ((flags & ctl_missing) && !g->sent_lost) {
            p = g->packet.data;
            *p++ = msg_lost;
            *p++ = j = g->parts_left > 255 ? 255 : g->parts_left;

            for (i = 0; j; i++)
                if (!g->data[g->size + i])
                    p = write16(p, i), j--;

            msg_send(g, p);
            g->sent_lost = 1;
        } else {
            g->sent_lost = 0;
        }

        if ((len -= 2) < 0)
            return;

        memcpy(&part_id, data, 2); data += 2;

        assert(part_id <= (g->size - 1) / part_size && len <= part_size);
        assert(len == part_size || part_id * part_size == g->size - len);

        if (!g->data[g->size + part_id]) {
            g->data[g->size + part_id] = 1;
            memcpy(g->data + part_id * part_size, data, len);

            if (!--g->parts_left) {
                printf("download complete\n");
                loadmap(g);

                p = g->packet.data;
                *p++ = msg_lost;
                *p++ = 0;
                msg_send(g, p);

                p = g->packet.data;
                *p++ = msg_loaded;
                msg_send(g, p);
            }
        }

        return;
    }

    recvgframe(g, flags & msg_bits, data, len);
    //assert();
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

    while ((len = net_recv(g->sock, &addr, &buf, sizeof(buf))) >= 0) {
        if (addr.cmp != g->addr.cmp)
            continue;

        if (buf.key & 0x8000) {
            if (g->connected)
                continue;

            if (len == 64 && !g->key)
                send_connect(g, &addr, buf.key);

            if (len != 12 || (buf.ckey & 0x7FFF) != g->ckey ||
                buf.inflated >= 0x1000000 || !buf.size || buf.size >= 0x800000)
                continue;

            parts = (buf.size - 1) / part_size + 1;
            data = malloc(buf.size + parts);
            if (!data)
                continue;

            memset(data + buf.size, 0, parts);

            g->packet.id = buf.id;
            g->packet.seq = 0;
            g->packet.frame = 0;
            g->packet.flags = 0;

            g->fps = buf.fps;
            g->rseq = 0;
            g->connected = 1;
            g->lastrecv = g->timer = g->time;

            g->size = buf.size;
            g->inflated = buf.inflated;
            g->parts_left = parts;
            g->data = data;

            printf("connected\n");
            continue;
        }

        if ((len -= 2) >= 0 && g->connected && buf.ckey == g->ckey)
            recvframe(g, buf.data + 2, len);
    }

    if (!g->addr.family)
        return;

    if (time_msec(g->time - g->lastrecv) >= 10000) //note: needs to be lower with fps > 20
        game_disconnect(g);

    if (time_msec(g->time - g->timer) >= 200) {
        g->timer = g->time;

        if (!g->key)
            directconnect(g, &g->addr);
        else if (!g->connected)
            send_connect(g, &g->addr, g->key);
        else
            net_send(g->sock, &g->addr, &g->packet, 4);
    }
}

void game_netorder(game_t *g, uint8_t order, uint8_t target, int8_t queue, uint8_t alt)
{
    uint8_t *p;

    p = g->packet.data;
    *p++ = msg_order;
    *p++ = g->nsel;
    *p++ = queue;
    *p++ = order;
    *p++ = target;
    *p++ = alt;

    if (target == target_pos)
        memcpy(p, &g->target_pos, 8), p += 8;
    else if (target == target_ent)
        memcpy(p, &g->target_ent, 2), p += 2;

    memcpy(p, g->sel_ent, g->nsel * 2); p += g->nsel * 2;

    msg_send(g, p);
}

bool game_netinit(game_t *g)
{
    g->sock = net_sock_nb();
    if (g->sock < 0)
        return 0;

    g->ckey = (time(0) & 0x7FFF);

    return 1;
}

//TODO entity replacement
