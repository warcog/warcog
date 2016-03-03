//#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

//#include "client.h"
//#include "game2.h"
//#include "gamedata.h"
#include "protocol.h"
#include "util.h"
#include "game.h"

#include <math.h>

static game_t game;

static uint8_t* playerdata(const game_t *g, uint8_t *p)
{
    uint8_t *pcount;
    int j;

    pcount = p++;

    j = 0;
    array_for_const(&g->cl, cl) {
        if (cl->status < cl_loaded)
            continue;
        memcpy(p, &cl->player, sizeof(cl->player)); p += sizeof(cl->player);
        j++;
    }

    *pcount = j;

    printf("pcount %u\n", j);

    return p;
}

static uint8_t* entitydata(game_t *g, client_t *cl, uint8_t *p, uint8_t frame, uint8_t df, bool reset)
{
    int i, j, id;
    centity_t *c;
    uint8_t nextframe, prevframe, dif, val;
    entity_t *ent;

    nextframe = frame + 1;
    prevframe = frame - 1;
    dif = frame - df; /* frames skipped */

    for (i = 0, j = 0; i < cl->nent; i++) {
        c = &cl->ent[id = cl->ents[i]];
        ent = array_get(&g->ent, id);

        if (reset) {
            if (c->lastseen == frame)
                p = ent_full(ent, id, p);
        } else {
            if (c->lastseen == frame) {
                if (val = frame - c->firstseen, dif >= val)
                    p = ent_full(ent, id, p);
                else
                    p = ent_frame(ent, id, p, frame, df);
            } else if (val = prevframe - c->lastseen, dif >= val) {
                p = ent_remove(ent, id, p);
            }
        }

        if (c->firstseen == nextframe)
            c->firstseen++;

        if (c->lastseen == nextframe)
            c->active = 0;
        else
            cl->ents[j++] = id;
    }

    cl->nent = j;

    return p;
}

static void addentity(client_t *cl, uint16_t id, uint8_t frame)
{
    centity_t *c;

    c = &cl->ent[id];

    if (c->active) {
        /* if it went out of vision, update firstseen value */
        if (c->lastseen != frame && c->lastseen != (uint8_t) (frame - 1))
            c->firstseen = frame;

        c->lastseen = frame;
        return;
    }

    c->active = 1;
    c->lastseen = c->firstseen = frame;
    cl->ents[cl->nent++] = id;
}

static bool checkentity(client_t *cl, uint16_t id, uint8_t frame)
{
    centity_t *c;

    c = &cl->ent[id];
    return (c->active && c->lastseen == frame);
}

static client_t* newclient(game_t *g, const addr_t *addr, const uint8_t *data)
{
    client_t *cl;
    uint8_t id, status;
    uint16_t key;

    status = data[0] ? cl_loading : cl_downloading;
    memcpy(&key, data + 2, 2);

    array_for(&g->cl, cl) {
        /* existing new connection with same ip/port and key */
        if (cl->status == status && cl->addr.cmp == addr->cmp && cl->key == key)
            return cl;

        /* reconnection */
        if (data[1] == cl_id(g, cl)) {
            if (cl->status == cl_disconnected && cl->key == key)
                goto reconnect;
            else
                return 0;
        }
    }

    if (data[1] != 0xFF) /* invalid reconnect */
        return 0;

    cl = array_new(&g->cl, &id);
    if (!cl)
        return 0;

    cl->key = key;
    cl->player.id = id;
    cl->player.team = 0xFF;

reconnect:
    cl->addr.cmp = addr->cmp;

    memcpy(cl->player.name, data + 4, name_size);

    cl->status = status;
    cl->timeout = 0;
    cl->seq = 0;
    cl->flags = 0;
    cl->frame = 0;
    cl->rate = 0;
    cl->part = 0;

    cl->nlost = 0;

    return cl;
}

static void joinclient(game_t *g, client_t *cl)
{
    uint8_t *p;

    player_join(g, &cl->player);
    g->info.players++;

    /* notify other players */
    array_for(&g->cl, c) {
        if (c->status < cl_loaded)// || c == cl)
            continue;

        p = event_write(&c->ev, g->frame, 1 + sizeof(cl->player));
        if (!p)
            assert(0);

        *p++ = ev_join;
        memcpy(p, &cl->player, sizeof(cl->player));
    }

    cl->status = cl_loaded_reset;
}

static void killclient(game_t *g, client_t *cl, uint8_t reason)
{
    uint8_t *p;
    bool remove;

    remove = 1;
    if (cl->status >= cl_loaded) {
        remove = player_leave(g, &cl->player);
        g->info.players--;
    }

    if (remove) { /* client removed */
        cl->status = cl_none;
        event_free(&cl->ev);
        cl->nent = 0;
        memset(cl->ent, 0, sizeof(cl->ent)); //

        array_remove(&g->cl, cl);
    } else {
        cl->status = cl_disconnected;
    }

    /* notify others */
    array_for(&g->cl, c) {
        if (c->status < cl_loaded || c == cl)
            continue;

        p = event_write(&c->ev, g->frame, 2);
        if (!p)
            assert(0);

        *p++ = ev_disconnect + ((reason << 1) | remove);
        *p++ = cl_id(g, cl);
    }
}

static void nonclient(game_t *g, const uint8_t *data, int len, const addr_t *addr)
{
    //TODO: send response on failure
    const client_t *cl;
    const ipentry_t *entry;

    entry = ip_find(&g->table, addr->ip);
    if (!entry)
        return;

    if (len == 63) { /* ping/getinfo */
        g->info.key = entry->key;
        memcpy(&g->info.value, data, 2);
        net_send(g->sock, addr, &g->info, sizeof(g->info));
        return;
    }

    if (len != (4 + name_size + 4))
        return;

    if (memcmp(data, &entry->key, 4))
        return;
    data += 4;

    cl = newclient(g, addr, data);
    if (!cl)
        return;

    g->conn_rep.key = cl->key | (ctl_notconn << 8);
    g->conn_rep.fps = FPS;
    g->conn_rep.id = cl_id(g, cl);
    net_send(g->sock, addr, &g->conn_rep, sizeof(g->conn_rep));
}

static void recvmsgs(game_t *g, client_t *cl, const uint8_t *data, int len)
{
    uint8_t msg;
    int i, j;

    do {
        msg = *data++;
        switch (msg) {
        case msg_setrate:
            if (--len < 0)
                return;

            cl->rate = *data++;
            if (cl->rate > max_rate)
                cl->rate = max_rate;
            break;
        case msg_lost:
            if (--len < 0)
                return;

            i = *data++;
            if ((len -= i * 2) < 0)
                return;

            memcpy(cl->lost, data, i * 2); data += i * 2;

            if (!i)
                cl->status = cl_loading;

            for (j = 0; j < i; j++)
                if (cl->lost[j] > maxpart)
                    i = 0;

            cl->nlost = i;
            cl->losti = 0;
            break;
        case msg_loaded:
            //cl->status = cl_loaded_reset;
            if (cl->status == cl_loading)
                joinclient(g, cl);
            break;
        case msg_chat:
            if (--len < 0)
                return;

            i = *data++ + 1;
            if (i > 128 || (len -= i) < 0)
                return;

            //chat(id, data, n);
            data += i;
            break;
        case msg_order:
            if ((len -= 5) < 0)
                return;

            i = *data++;
            if ((len -= (i * 2)) < 0)
                return;

            {
                order_t order;
                uint16_t id;
                int8_t queue;
                entity_t *ent;

                queue = *data++;

                order.id = *data++;
                order.target_type = *data++;
                order.alt = *data++;
                order.timer = 0;

                switch (order.target_type) {
                case target_none:
                    break;
                case target_pos:
                    if ((len -= 8) < 0)
                        return;
                    memcpy(&order.p, data, 8); data += 8;
                    break;
                case target_ent:
                    if ((len -= 2) < 0)
                        return;
                    memcpy(&order.ent_id, data, 2); data += 2;
                    if (order.ent_id == 0xFFFF || array_get(&g->ent, order.ent_id)->def < 0)
                        return;
                    break;
                default:
                    return;
                }

                while (i--) {
                    memcpy(&id, data, 2); data += 2;
                    if (id == 0xFFFF)
                        continue;

                    ent = array_get(&g->ent, id);
                    if (ent->def < 0 || ent->owner != cl_id(g, cl) || !def(ent)->control)
                        continue;

                    if (!queue) { /* no queue */
                        ent->order[0] = order;
                        ent->norder = 1;
                    } else if (queue < 0) { /* front queue */
                        if (ent->norder == 16)
                            ent->norder--;

                        memmove(ent->order + 1, ent->order, ent->norder * sizeof(*ent->order));
                        ent->order[0] = order;
                        ent->norder++;
                    } else { /* back queue */
                        if (ent->norder == 16)
                            continue;
                        ent->order[ent->norder++] = order;
                    }
                }
            }
            break;
        case msg_disconnect:
            killclient(g, cl, disconnected);
            return;
        default:
            return;
        }

    } while (--len >= 0);
}

void on_recv(void)
{
    game_t *g = &game;
    client_t *cl;
    uint8_t id, seq, flags;

    addr_t addr;
    int len;
    uint8_t buf[65536];
    len = net_recv(g->sock, &addr, buf, sizeof(buf));
    uint8_t *data = buf;

    if ((len -= 4) < 0) /* minimum length for valid packet is 4 */
        return;

    id = *data++;
    if (id == 0xFF) {
        nonclient(g, data, len + 3, &addr);
        return;
    }

    cl = array_get(&g->cl, id);
    if (cl->status <= cl_disconnected || cl->addr.cmp != addr.cmp)
        return;

    seq = *data++;
    cl->frame = *data++;
    flags = *data++;

    if (cmpbit(flags, cl->flags, ctl_out)) {
        if (cl->status == cl_loaded)
            cl->status = cl_loaded_delta;

        cl->flags ^= ctl_out;
    }

    if (cmpbit(flags, cl->flags, ctl_in))
        return;

    if (seq != cl->seq) {
        cl->flags ^= ctl_in;
        return;
    }

    cl->timeout = 0;
    if (--len >= 0) {
        cl->seq++;
        recvmsgs(g, cl, data, len);
    }
}

static void clientframe(game_t *g)
{
    uint32_t i, offset, part, size;
    uint8_t data[65536], *p, msg, frame;
    bool waiting;

    //TODO: rate
    i = 0;
    array_for(&g->cl, cl) {
        if (cl->status <= cl_disconnected)
            continue;

        if (++cl->timeout == cl_timeout) {
            printf("client timed out\n");
            killclient(g, cl, timeout);
            continue;
        }
        i = array_keep(&g->cl, cl, i);

        p = data;
        memcpy(p, &cl->key, 2); p += 2;
        *p++ = cl->seq;

        if (cl->status <= cl_loading) {
            if (cl->status == cl_downloading) {
                part = cl->part;
                if (part > maxpart) {
                    if (cl->losti != cl->nlost) {
                        part = cl->lost[cl->losti++];
                    } else {
                        waiting = 1;
                        size = 0;
                        goto empty;
                    }

                    waiting = (cl->losti == cl->nlost);
                } else {
                    cl->part++;
                    waiting = (part == maxpart);
                }

                offset = part * part_size;
                size = g->conn_rep.size - offset;
                if (size > part_size)
                    size = part_size;

            empty:
                *p++ = msg_data | cl->flags | (waiting ? ctl_missing : 0);
                p = write16(p, part);
                memcpy(p, g->data + offset, size);
                p += size;
            } else {
                *p++ = msg_data | cl->flags;
            }
        } else {
            msg = (cl->status & 3);
            *p++ = msg | cl->flags;
            *p++ = g->frame;

            frame = g->frame;
            if (msg == msg_delta)
                *p++ = frame = cl->frame;

            if (msg == msg_reset) {
                p = playerdata(g, p);
                memcpy(p, &cl->cam_pos, sizeof(cl->cam_pos)); p += sizeof(cl->cam_pos);
            } else {
                p = event_copy(&cl->ev, p, frame, g->frame), *p++ = ev_end;
            }

            p = entitydata(g, cl, p, g->frame, frame, msg == msg_reset);
            event_clear(&cl->ev, g->frame + 1);
            cl->status = cl_loaded;
        }
        net_send(g->sock, &cl->addr, data, p - data);
    }
    array_update(&g->cl, i);
}

static void turn(entity_t *ent, point p)
{
    ent->wt = p;
    ent->turn = 1;
}

static void walk(entity_t *ent, point p, uint32_t range)
{
    ent->wt = p;
    ent->wt_range = range;
    ent->walk = 1;
}

static int entityorderframe(game_t *g, entity_t *ent, order_t *order)
{
    entity_t *target;
    ability_t *a;
    target_t t;
    uint8_t id, i;
    cast_t cast;

    t.type = order->target_type;
    t.alt = order->alt;
    if (t.type == target_ent) {
        target = array_get(&g->ent, order->ent_id);
        if (target->delete)
            return 0;

        t.ent = order->ent_id;
        t.pos = target->pos;
    } else {
        t.pos = order->p;
    }

    if (order->id & 128) {
        id = (order->id & 127);
        i = 0;
        do {
            if (i == ent->nability)
                return 0;
            a = &ent->ability[i++];
        } while (a->id != id);

        if (!(def(a)->target & (1 << t.type)))
            return 0;

        if (!ability_allowcast(g, ent, a, t))
            return 0;

        if (!order->timer || !ability_continuecast(g, ent, a, &t)) {
            cast = ability_startcast(g, ent, a, &t);
            if (!cast.value) {
                return 0;
            }

            if (cast.value < 0) {
                if (cast.value == -1) {
                    turn(ent, t.pos);
                    setanim(g, ent, anim_walk, def(ent)->walk_time, 0, 0);
                } else if (cast.value == -2) {
                    walk(ent, t.pos, cast.range);
                }

                order->timer = 0;
                return 1;
            }

            if (cast.cast_time)
                setanim(g, ent, cast.anim, cast.anim_time, 1, 0);
            order->timer = cast.cast_time;

            for (i = 0; i < ent->nability; i++)
                ability_onabilitystart(g, ent, &ent->ability[i], a, t);
        }

        if (!order->timer || !--order->timer) {
            ability_onpreimpact(g, ent, a, t);
            for (i = 0; i < ent->nability; i++)
                ability_onabilitypreimpact(g, ent, &ent->ability[i], a, t);

            return ability_onimpact(g, ent, a, t) ? 1 : (cast.cast_time ? 0 : -1);
        }

        if (t.type != target_none)
            turn(ent, t.pos);
        return 1;
    }

    switch (order->id) {
    case 0:
        if (order->target_type == target_pos) {
            if (inrange(ent->pos, t.pos, 0))
                return 0;

            walk(ent, t.pos, 0);
            return 1;
        }

        if (order->target_type == target_ent) {
            if (!inrange(ent->pos, t.pos, distance(8)))
                walk(ent, t.pos, 0);
            else
                setanim(g, ent, anim_idle, def(ent)->idle_time, 0, 0);
            return 1;
        }
        return 0;
    case 1: /* stop */
    case 2: /* hold */
    default:
        return 0;
    }
}

#define state_iterate(g, ent, func) \
{ \
    int index; \
    state_t *state; \
    for (index = 0; index < ent->nstate; index++) { \
        state = &ent->state[index]; \
        func; \
    } \
}

#define state_iterate_new(g, ent, func) \
{ \
    int index; \
    state_t *state; \
    for (index = 0; index < ent->nstate_new; index++) { \
        state = &ent->state[index]; \
        func; \
    } \
}

static void entityframe(game_t *g, entity_t *ent)
{
    int res;
    uint8_t i, j;
    bool acted;

    if (ent->anim.id & 128)
        return;

    if (!ent->anim_set)
        ent->anim.frame++;

    for (i = 0, j = 0, acted = 0; i < ent->norder; i++) {
        res = acted ? 1 : entityorderframe(g, ent, &ent->order[i]);
        acted = (res >= 0);
        if (res > 0)
            ent->order[j++] = ent->order[i];
    }
    ent->norder = j;

    if (!ent->norder && ent->anim_cont)
        setanim(g, ent, anim_idle, def(ent)->idle_time, 0, 0);

    if (ent->anim.frame == ent->anim.len) {
        ent->anim.frame = 0;
        ent->anim_forced = 0;

        //state_iterate(g, ent, state_onanimfinish(g, ent, state));

        if (!ent->norder)
            setanim(g, ent, anim_idle, def(ent)->idle_time, 0, 0);
    }
}

static void abilitypostframe(game_t *g, entity_t *ent)
{
    ability_t *a;
    uint8_t i, j, k, max;

    ent->ability_history[g->frame & 0xFF] = ent->nability_new;

    for (i = 0, j = 0, max = ent->nability_new; i < max; i++) {
        a = &ent->ability[i];
        if (a->delete) {
            ent->ability_freeid[--ent->nability_new] = a->id;
            ent->ability_freetime[a->id] = g->frame;
            ent->update.ability = g->frame;
            continue;
        }

        for (k = 0; k < sizeof(a->update); k++)
            if (a->update.value[k] == (uint8_t)(g->frame + 1))
                a->update.value[k]++;

        ent->ability[j++] = *a;
    }

    ent->nability = ent->nability_new = j;
}

static void statepostframe(game_t *g, entity_t *ent)
{
    state_t *s;
    uint8_t i, j, k, max;

    ent->state_history[g->frame & 0xFF] = ent->nstate_new;

    for (i = 0, j = 0, max = ent->nstate_new; i < max; i++) {
        s = &ent->state[i];
        if (s->delete) {
            ent->state_freeid[--ent->nstate_new] = s->id;
            ent->state_freetime[s->id] = g->frame;
            ent->update.state = g->frame;
            continue;
        }

        for (k = 0; k < sizeof(s->update); k++)
            if (s->update.value[k] == (uint8_t)(g->frame + 1))
                s->update.value[k]++;

        ent->state[j++] = *s;
    }

    ent->nstate = ent->nstate_new = j;
}

static void gameframe(game_t *g)
{
    int j, k;
    entity_t *source;
    uint32_t prev_maxhp, prev_maxmana, prev_vision;
    point p;
    delta del;
    double d;
    int16_t dif;
    uint16_t angle, turnrate;

    g->update_map = 0;

    array_for(&g->ent, ent) {
        entityframe(g, ent);

        entity_onframe(g, ent);

        for_ability(ent, a)
            ability_onframe(g, ent, a);

        state_iterate(g, ent, state_onframe(g, ent, state));
    }

    array_for(&g->cl, cl)
        if (cl->status >= cl_loaded)
            player_frame(g, &cl->player);

    global_frame(g);

    /* post frame: stats update, motion, walking, death */
    array_for(&g->ent, ent) {
        prev_maxhp = ent->maxhp;
        prev_maxmana = ent->maxmana;
        prev_vision = ent->vision;

        entity_initmod(g, ent);

        /* recalculate stats */
        //entity_applymod(ent);
        //ability_call(ent, a, j, ability_applymod(ent, a));
        //state_call(ent, s, j, state_applymod(ent, s));

        state_iterate_new(g, ent,
            state_applymod(g, ent, state);

            if (state->delete)
                continue;

            if (state->lifetime && !--state->lifetime) {
                state_onexpire(g, ent, state);
                state->delete = 1;
            }
        );

        for_ability_new(ent, a) {
            if (a->cd)
                a->cd--;

            ability_finishmod(g, ent, a);
        }


        entity_finishmod(g, ent);

        if (ent->vision != prev_vision)
            ent->update.info = g->frame;

        if (ent->tp) {
            ent->pos = ent->teleport;
            ent->update.pos = g->frame;
            ent->tp = 0;
        }

        if (ent->push.x || ent->push.y)
            ent->update.pos = g->frame;

        ent->pos.x += ent->push.x;
        ent->pos.y += ent->push.y;
        ent->push.x = ent->push.y = 0;

        if ((int) ent->pos.x < 0)
            ent->pos.x = 0;
        if ((int) ent->pos.y < 0)
            ent->pos.y = 0;
        if (ent->pos.x > max_coord)
            ent->pos.x = max_coord;
        if (ent->pos.y > max_coord)
            ent->pos.y = max_coord;

        turnrate = def(ent)->turnrate;

        //def(ent)->radius
        if (ent->walk && map_path(&g->map, &p, ent->pos, ent->wt, 0, ent->wt_range)) {
            angle = getangle(ent->pos, p);
            if (facing(ent->angle, angle, 16384)) {
                del = delta(ent->pos, p);
                d = sqrt(mag2(del));
                if (d <= ent->movespeed)
                    ent->pos = p;
                else
                    ent->pos = addvec(ent->pos, scale(vec(del.x, del.y), ent->movespeed / d));
            }

            ent->update.pos = g->frame;
            setanim(g, ent, anim_walk, def(ent)->walk_time, 0, 0);
        } else {
            p = ent->wt;
        }

        if (ent->walk || ent->turn) {
            angle = getangle(ent->pos, p);
            dif = angle - ent->angle;
            if (dif) {
                ent->update.pos = g->frame;
                if (abs(dif) < turnrate) {
                    ent->angle = angle;
                } else if (dif > 0) {
                    ent->angle += turnrate;
                } else {
                    ent->angle -= turnrate;
                }
            }

            ent->walk = ent->turn = 0;
        }

        /* healing and damage */
        ent->hp = prev_maxhp ? ((uint64_t) ent->hp * ent->maxhp) / prev_maxhp : ent->maxhp;
        ent->mana = prev_maxmana ? ((uint64_t) ent->mana * ent->maxmana) / prev_maxmana : ent->maxmana;

        if (ent->refresh) {
            ent->damage = 0;
            ent->heal = ent->maxhp - ent->hp;
            ent->mana = ent->maxmana;
            ent->refresh = 0;
        }

        if (ent->maxhp != prev_maxhp || ent->damage != ent->heal)
            ent->update.hp = g->frame;

        if (ent->maxmana != prev_maxmana)
            ent->update.mana = g->frame;

        ent->hp += ent->heal; //

        if (ent->hp <= ent->damage) { /* death */
            ent->hp = 1;

            source = array_get(&g->ent, ent->damage_source);

            state_iterate(g, ent, if (state_ondeath(g, ent, state, source)) goto cancel);

            for_ability_new(ent, a)
                if (ability_ondeath(g, ent, a, source))
                    goto cancel;

            entity_ondeath(g, ent, source);
        } else {
            ent->hp -= ent->damage;
        }
    cancel:
        ent->damage = 0;
        ent->damage_max = 0;
        ent->heal = 0;

        if (ent->hp > ent->maxhp)
            ent->hp = ent->maxhp;
    }
    array_update2(&g->ent);

    map_entity_init(&g->map);

    j = 0;
    array_for(&g->ent, ent) {
        if (ent->delete) {
            array_remove(&g->ent, ent);
            ent->def = -1;
            continue;
        }
        map_entity_add(&g->map, ent->pos, ent_id(g, ent), j);
        j = array_keep(&g->ent, ent, j);


        ent->anim_set = 0;
        if (ent->proxy != 0xFFFF) {
            if (array_get(&g->ent, ent->proxy)->delete) {
                ent->proxy = 0xFFFF;
                ent->update.info = g->frame;
            }
        }

        abilitypostframe(g, ent);
        statepostframe(g, ent);
        //validate source
        //validate target

        //ability cooldown, validate target
        //state  ...stuff

        for (k = 0; k != sizeof(ent->update); k++)
            if (ent->update.value[k] == (uint8_t)(g->frame + 1))
                ent->update.value[k]++;
    }
    array_update(&g->ent, j);

    map_entity_finish(&g->map, j);


    //TODO optimize and use alliance system
    array_for(&g->cl, cl) {
        array_for(&g->ent, ent) {
            if (ent->team != cl->player.team)
                continue;

            //for each entity ent which shares vision with player cl
            addentity(cl, ent_id(g, ent), g->frame);
            if (!ent->vision) /* entity gives no vision */
                continue;

            map_vision_init(&g->map, ent->pos, ent->vision);

            areaofeffect(g, ent->pos, ent->vision, 0xFF,
                if (target == ent) //check not needed?
                    continue;

                //if (vision_blocking)
                //    addblocker();

                if (checkentity(cl, ent_id(g, target), g->frame)) /* already added this frame */
                    continue;

                if (!entity_visible(g, target, ent)) /* */
                    continue;

                //TODO here add entity to a list and do vision block checks in another loop

                if (!map_vision_check(&g->map, target->pos)) /* trees and cliffs */
                    continue;

                addentity(cl, ent_id(g, target), g->frame);
            );
        }
    }

    if (g->update_map)
        map_build(&g->map);
}

void on_timer(void)
{
    game_t *g = &game;
    uint32_t key;

    if (!(g->frame & 127)) { /* every 128 frames */
        key = 3273279;
        net_send(g->sock, &g->master_addr, &key, 4);
    }

    gameframe(g);
    clientframe(g);

    g->frame++;
}

bool on_init(int argc, const char *argv[], int sock)
{
    game_t *g = &game;

    g->sock = sock;
    if (argc >= 3)
        net_getaddr(&g->master_addr, argv[1], argv[2]);

    array_init(&g->cl);
    array_init(&g->ent);

    if (!file_read("datamap", &g->map, map_file_size)) {
        printf("datamap missing or wrong size\n");
        return 0;
    }

    if (!file_read("data", g->data, sizeof(g->data))) {
        printf("data missing or wrong size\n");
        return 0;
    }

    map_build(&g->map);

    g->conn_rep.size = compressed_size;
    g->conn_rep.inflated = inflated_size;

    //strcpy((char*) g->info.name, (char*) mapdef.name);
    //strcpy((char*) g->info.desc, (char*) mapdef.desc);
    g->info.maxplayers = 255;

    return 1;
}
