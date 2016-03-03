#include <net.h>
#include <array.h>

#include <ip.h>
#include <stdio.h>

//TODO 16-bit server->client seq so it doesnt break when >=256 messages are missed

typedef struct {
    addr_t addr;
    uint8_t connected, timeout, seq_off;

    uint8_t seq_c;

    uint8_t gl_req, gl_seq;

    union {
        struct {
            uint8_t flags, unused, seq_out, seq_in;
        };
        uint32_t header;
    };
} client_t;

typedef struct {
    uint8_t timeout, unused;
    uint16_t port;
    uint32_t ip;
} game_t;

typedef struct {
    int sock;

    uint8_t seq_out;

    array(client_t, uint32_t, 65536) cl;
    sarray(game_t, uint8_t, 255) gl;

    uint32_t buf_offset[256];
    uint8_t buf[65536];

    iptable_t table;
} chatserver_t;

chatserver_t global;

static bool inrange(uint8_t a, uint8_t b, uint8_t k)
{
    k -= a;
    b -= a;
    return (k < b);
}

static void sendping(chatserver_t *g, client_t *cl)
{
    net_send(g->sock, &cl->addr, &cl->header, sizeof(cl->header));
}

static void addmessage(chatserver_t *g, uint8_t *data, size_t len)
{
    size_t start, end;
    //netbuf_t buf[2];

    //if (len > 256)
    //    printf("large len shouldnt happen\n");

    //if (((g->seq_out + 2) & 0xFF) == g->seq_c)
    //    return 0;

    start = g->buf_offset[(g->seq_out + 1) & 0xFF];
    end = g->buf_offset[g->seq_out];
    //if ((((end - start) & 0xFFFF) + len) & 0x10000) //

    start = end;
    end = (end + len) & 0xFFFF;

    g->seq_out++;
    g->buf_offset[g->seq_out] = end;

    if (start <= end) {
        memcpy(g->buf + start, data, len);
    } else {
        memcpy(g->buf + start, data, sizeof(g->buf) - start);
        memcpy(g->buf, data + (sizeof(g->buf) - start), end);
    }
}

static void resend(chatserver_t *g, client_t *cl)
{
    size_t len, start, end, i;
    netbuf_t buf[4];

    start = g->buf_offset[(cl->seq_c - cl->seq_off) & 0xFF];
    end = g->buf_offset[g->seq_out];
    len = (end - start) & 0xFFFF;

    //cl->seq_out = cl->seq_c + 1;
    //g->buf_offset[cl->seq_out] = end;

    cl->seq_out = cl->seq_c;

    i = 0;
    buf[i++] = net_buf(&cl->header, 4);
    if (start <= end) {
        buf[i++] = net_buf(g->buf + start, len);
    } else {
        buf[i++] = net_buf(g->buf + start, sizeof(g->buf) - start);
        buf[i++] = net_buf(g->buf, end);
    }

    if (cl->gl_req)
        buf[i++] = net_buf(&g->gl, 4 + g->gl.scount * 8);

    net_sendmsg(g->sock, &cl->addr, buf, i);
    cl->seq_out = g->seq_out + cl->seq_off;
}

static void updategame(chatserver_t *g, const addr_t *addr)
{
    game_t *p;

    sarray_for(&g->gl, p) {
        if (p->ip == addr->ip && p->port == addr->port) {
            p->timeout = 0;
            return;
        }
    }

    p = sarray_new(&g->gl);
    if (!p)
        return;

    p->port = addr->port;
    p->ip = addr->ip;
}

bool on_init(int argc, const char *argv[], int sock)
{
    chatserver_t *g = &global;
    (void) argc;
    (void) argv;

    g->sock = sock;
    array_init(&g->cl);

    g->gl.val = 3;

    return 1;
}

void on_timer(void)
{
    chatserver_t *g = &global;
    uint32_t i, id;
    uint8_t data[4 + 256];

    data[4] = 2;
    data[5] = 0;

    i = 0;
    array_for(&g->cl, cl) {
        if (cl->timeout == 10) {
            if (data[5] == 63)
                continue; //TODO

            id = array_id(&g->cl, cl);
            memcpy(&data[6 + data[5] * 4], &id, 4);
            data[5]++;

            array_remove(&g->cl, cl);
            continue;
        }
        cl->timeout++;

        sendping(g, cl);
        i = array_keep(&g->cl, cl, i);
    }
    array_update(&g->cl, i);

    if (data[5]) {
        addmessage(g, data + 4, 2 + data[5] * 4);
        array_for(&g->cl, c) {
            memcpy(data, &c->header, 4);
            net_send(g->sock, &c->addr, data, 6 + data[5] * 4l);
            c->seq_out = g->seq_out + c->seq_off;
        }
    }

    i = 0;
    sarray_for(&g->gl, p) {
        if (++p->timeout == 60)
            continue;

        i = sarray_keep(&g->gl, p, i);
    }
    sarray_update(&g->gl, i);
}

uint32_t newclient(chatserver_t *g, const addr_t *addr)
{
    uint32_t id;
    client_t *cl;

    cl = array_new(&g->cl, &id);
    if (!cl)
        return ~0u;

    cl->addr = *addr;

    cl->connected = 1;
    cl->timeout = 0;
    cl->seq_off = 0;

    cl->gl_req = 0;

    cl->header = 0;

    /* */
    uint8_t data[4 + 6];
    data[4] = 1;
    memcpy(&data[5], &id, 4);
    data[9] = 0;
    addmessage(g, data + 4, 6);
    array_for(&g->cl, c) {
        if (array_id(&g->cl, c) == id)
            continue;

        memcpy(data, &c->header, 4);
        net_send(g->sock, &c->addr, data, 10);
        c->seq_out = g->seq_out + c->seq_off;
    }

    cl->seq_out = g->seq_out;

    return id;
}

void on_recv(void)
{
    chatserver_t *g = &global;

    ipentry_t *entry;
    client_t *cl;
    addr_t addr;
    int len;
    uint8_t *p, msg, val;
    struct {
        union {
            uint32_t key;
            struct {
                uint32_t id;
                uint8_t flags, seq_out, seq_in, unused2;
                uint8_t data[65536 - 8];
            };
            struct {
                uint8_t flag, unused1, seq_out2, unused3;
                uint32_t val;
            };
        };
    } buf;

    len = net_recv(g->sock, &addr, &buf, sizeof(buf));
    if (len < 0)
        return;

    if (len == 4) {
        if (buf.key == 3273279) {
            updategame(g, &addr); //
            return;
        }

        entry = ip_find(&g->table, addr.ip);
        if (!entry)
            return;

        if (entry->key != buf.key) {
            buf.flag = 4;
            buf.val = entry->key;
        } else {
            cl = array_get(&g->cl, entry->value);
            if (entry->value == ~0u || !cl->connected || cl->addr.cmp != addr.cmp)
                entry->value = newclient(g, &addr);

            buf.flag = 8;
            buf.seq_out2 = g->seq_out;
            buf.val = entry->value;
        }

        net_send(g->sock, &addr, &buf.flag, 8);
        return;
    }

    if ((len -= 8) < 0)
        return;

    if (buf.id >= array_max(&g->cl))
        return;

    cl = array_get(&g->cl, buf.id);
    if (!cl->connected || cl->addr.cmp != addr.cmp)
        return;

    //printf("(%u %u) (%u %u) (%u %u %u) (%i)\n", buf.flags, cl->flags, buf.seq_in, cl->seq_in,
    //       buf.seq_out, cl->seq_c, cl->seq_out, len);

    if (cl->gl_req)
        if (inrange(cl->seq_c, buf.seq_out, cl->gl_seq))
            cl->gl_req = 0;

    cl->seq_c = buf.seq_out; //TODO validate

    if ((buf.flags & 2) != (cl->flags & 2))
        cl->flags ^= 2, resend(g, cl);

    if ((buf.flags & 1) != (cl->flags & 1))
        return;

    if (buf.seq_in != cl->seq_in) {
        cl->flags ^= 1;
        sendping(g, cl); //TODO: merge with resend when both
        return;
    }

    cl->timeout = 0;

    if (!len)
        return;

    cl->seq_in++;

    p = buf.data;
    uint8_t data[4 + 256];
    while (--len >= 0) {
        switch (msg = *p++) {
        case 0: /* chat msg */
        case 1: /* user join/name */
            if (--len < 0)
                return;
            val = *p++;
            if (6 + val > 256)
                return;
            if ((len -= val) < 0)
                return;

            data[4] = msg;
            memcpy(&data[5], &buf.id, 4);
            data[9] = val;
            memcpy(&data[10], p, val);

            addmessage(g, data + 4, 6 + val);
            array_for(&g->cl, c) {
                memcpy(data, &c->header, 4);
                net_send(g->sock, &c->addr, data, 10 + val);
                c->seq_out = g->seq_out + c->seq_off;
            }
            p += val;
            break;
        case 2: /* user leave */
            break;
        case 3: /* game list */ {
            netbuf_t buf[2];
            cl->gl_req = 1;
            cl->gl_seq = cl->seq_out;

            buf[0] = net_buf(&cl->header, 4);
            buf[1] = net_buf(&g->gl, 4 + g->gl.scount * 8);
            net_sendmsg(g->sock, &cl->addr, buf, 2);

            cl->seq_off++;
            cl->seq_out = g->seq_out + cl->seq_off;
        }   break;
        default:
            return;
        }
    }
}
