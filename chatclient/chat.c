#include "chat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <array.h>
#include <util.h>

static void gameping(chat_t *chat, game_t *g)
{
    addr_t addr;
    uint8_t data[64];

    data[0] = 0xFF;
    data[1] = chat->pingid;
    data[2] = g - chat->list;

    net_addr_ipv4(&addr, g->addr.ip, g->addr.port);
    net_send(chat->sock, &addr, data, sizeof(data));
}

static void sendconnect(chat_t *chat)
{
    net_send(chat->sock, &chat->addr, &chat->key, 4);
}

static void sendping(chat_t *chat)
{
    net_send(chat->sock, &chat->addr, &chat->id, 8);
}

static bool sendmessage(chat_t *chat, uint8_t *data, size_t len)
{
    size_t start, end;
    netbuf_t buf[2];

    if (((chat->seq_out + 2) & 0xFF) == chat->seq_c)
        return 0;

    start = chat->buf_offset[chat->seq_c];
    end = chat->buf_offset[chat->seq_out];
    if (((end - start) & 0xFFFF) + len >= 65536) //16bit overflow
        return 0;

    start = end;
    end = (end + len) & 0xFFFF;

    chat->buf_offset[(chat->seq_out + 1) & 0xFF] = end;

    if (start <= end) {
        memcpy(chat->buf + start, data, len);
    } else {
        memcpy(chat->buf + start, data, 65536 - start);
        memcpy(chat->buf, data + (65536 - start), end);
    }

    buf[0] = net_buf(&chat->id, 8);
    buf[1] = net_buf(data, len);
    net_sendmsg(chat->sock, &chat->addr, buf, 2);
    chat->seq_out++;
    return 1;
}

static void resend(chat_t *chat)
{
    size_t len, start, end;
    netbuf_t buf[3];

    start = chat->buf_offset[chat->seq_c];
    end = chat->buf_offset[chat->seq_out];
    len = (end - start) & 0xFFFF;

    chat->seq_out = chat->seq_c;
    chat->buf_offset[(chat->seq_out + 1) & 0xFF] = end;

    buf[0] = net_buf(&chat->id, 8);
    if (start <= end) {
        buf[1] = net_buf(chat->buf + start, len);
        net_sendmsg(chat->sock, &chat->addr, buf, 2);
    } else {
        buf[1] = net_buf(chat->buf + start, 65536 - start);
        buf[2] = net_buf(chat->buf, end);
        net_sendmsg(chat->sock, &chat->addr, buf, 3);
    }
    chat->seq_out++;
}

bool chat_init(chat_t *chat, void *obj)
{
    int sock;

    //if (!net_getaddr(&chat->addr, "dota.warcog.org", "1336"))
    if (!time_init())
        return 0;

    if (!net_getaddr(&chat->addr, "127.0.0.1", "1336"))
        return 0;

    sock = net_sock();
    if (sock < 0 || !net_attach(sock, obj))
        return 0;

    chat->sock = sock;
    chat->obj = obj;

    sendconnect(chat);
    return 1;
}

void chat_free(chat_t *chat)
{
    net_close(chat->sock);
}

void chat_recv(chat_t *chat)
{
    addr_t addr;
    int len;
    uint8_t *p, val, msg;
    uint32_t id;
    struct {
        uint8_t flags, unused, seq_in, seq_out;
        union {
            uint32_t key;
            struct {
                uint32_t id;
            };
            uint8_t data[65536 - 4];
        };
    } buf;// __attribute__ ((aligned (16));
    gameinfo_t *info = (void*) &buf;
    game_t *g;

    len = net_recv(chat->sock, &addr, &buf, sizeof(buf));
    if ((len -= 4) < 0)
        return;

    if (len == 60 && (info->key & 0x8000)) {
        if (info->id >= chat->ngame || !info->key)
            return;

        g = &chat->list[info->id];
        if (g->info.key || addr.ip != g->addr.ip || addr.port != g->addr.port)
            return;

        memcpy(&g->info, info, sizeof(*info));
        g->info.ping = time_msec(time_get() - chat->ping_time[info->value & 15]);

        chat->sort[chat->npinged] = info->id;
        chat->npinged++;

        chat_gamelist_cb(chat);
        return;
    }

    if (buf.flags & 4) {
        if (len != 4)
            return;
        chat->key = buf.key;
        return;
    } else if (buf.flags & 8) {
        if (len != 4 || chat->connected)
            return;

        chat->seq_in = buf.seq_in;
        chat->id = buf.id;
        chat->connected = 1;
        chat->timeout = 0;
        chat_status_cb(chat);
        return;
    }

    //printf("(%u %u) (%u %u) (%u %u %u) (%i)\n", buf.flags, chat->flags, buf.seq_in, chat->seq_in,
    //       buf.seq_out, chat->seq_c, chat->seq_out, len);

    chat->seq_c = buf.seq_out; //validate

    if ((buf.flags & 1) != (chat->flags & 1)) /* client->server break */
        chat->flags ^= 1, resend(chat);

    if ((buf.flags & 2) != (chat->flags & 2)) /* wrong server->client bit */
        return;

    if (buf.seq_in != chat->seq_in) { /* server->client break */
        chat->flags ^= 2;
        sendping(chat); //TODO: merge with resend when both
        return;
    }

    chat->timeout = 0;

    if (!len)
        return;

    p = buf.data;
    while (--len >= 0) {
        switch (msg = *p++) {
        case 0: /* chat msg */
        case 1: /* user join/name */
            if ((len -= 5) < 0)
                return;
            memcpy(&id, p, 4); p += 4;
            val = *p++;
            if ((len -= val) < 0)
                return;

            if (msg == 0)
                chat_msg_cb(chat, id, (char*) p, val);
            else
                chat_name_cb(chat, id, (char*) p, val);
            p += val;
            break;
        case 2: /* user leave */
            if (--len < 0)
                return;

            val = *p++;
            if ((len -= val * 4) < 0)
                return;

            while (val--) {
                memcpy(&id, p, 4); p += 4;
                chat_leave_cb(chat, id);
            }
            break;
        case 3: /* game list */
            if ((len -= 3) < 0)
                return;
            val = *p; p += 3;
            if ((len -= val * 8) < 0)
                return;

            chat->ngame = val;
            chat->npinged = 0;
            chat->pingtimeout = 0;

            carray_for(chat->list, g, chat->ngame) {
                memcpy(&g->addr, p, 8), p += 8;
                g->info.key = 0;
                gameping(chat, g);
            }

            chat->ping_time[chat->pingid] = time_get();
            chat->pingid = (chat->pingid + 1) & 15;

            chat_gamelist_cb(chat);
            break;
        default:
            return;
        }
        chat->seq_in++;
    }
}

void chat_timer(chat_t *chat)
{
    if (!chat->connected) {
        sendconnect(chat);
        return;
    }

    if (++chat->pingtimeout < 16) {
        carray_for(chat->list, g, chat->ngame)
            gameping(chat, g);

        chat->ping_time[chat->pingid] = time_get();
        chat->pingid = (chat->pingid + 1) & 15;
    }

    if (++chat->timeout == 10) {
        chat->connected = 0;
        chat_status_cb(chat);
        return;
    }

    sendping(chat);
}

bool chat_name(chat_t *chat, const char *name, uint8_t len)
{
    uint8_t data[2 + len];

    data[0] = 1;
    data[1] = len;
    memcpy(data + 2, name, len);

    return sendmessage(chat, data, sizeof(data));
}

bool chat_send(chat_t *chat, const char *msg, uint8_t len)
{
    uint8_t data[2 + len];

    data[0] = 0;
    data[1] = len;
    memcpy(data + 2, msg, len);

    return sendmessage(chat, data, sizeof(data));
}

bool chat_gamelist_refresh(chat_t *chat)
{
    uint8_t byte = 3;
    return sendmessage(chat, &byte, 1);
}
