#include "conn.h"
#include <time.h>
#include <util.h>

bool conn_init(conn_t *conn)
{
    int sock;

    sock = net_sock_nb();
    if (sock < 0)
        return 0;

    conn->sock = sock;
    conn->ckey = (time(0) & 0x7FFF);
    return 1;
}

void send_connect(conn_t *conn, uint64_t time)
{
    union {
        struct {
            uint8_t ones;
            uint8_t key[4];
            uint8_t have_map, id;
            uint8_t ckey[2];
            uint8_t name[16];
        };
        struct {
            uint32_t ping[16];
        };
    } msg;

    conn->timer = time;

    if (conn->key) {
        msg.ones = 0xff;
        memcpy(msg.key, &conn->key, 4);
        strcpy((char*) msg.name, "newbie");
        msg.have_map = 0;
        msg.id = 0xFF;
        memcpy(msg.ckey, &conn->ckey, 2);

        net_send(conn->sock, &conn->addr, &msg, 25);
    } else {
        msg.ping[0] = ~0u;
        net_send(conn->sock, &conn->addr, &msg, 64);
    }
}

void conn_connect(conn_t *conn, const addr_t *addr, uint32_t key, uint64_t time)
{
    conn->connected = 0;
    conn->ckey = (conn->ckey + 1) & 0x7FFF;
    conn->lastrecv = time;
    conn->addr.cmp = addr->cmp;
    conn->key = key;
    conn->active = 1;

    send_connect(conn, time);
}

void conn_disconnect(conn_t *conn)
{
    conn->active = 0;
}

bool conn_frame(conn_t *conn, uint64_t time)
{
    if (!conn->active)
        return 1;

    if (time_msec(time - conn->lastrecv) >= 5000)
        return 0;

    if (time_msec(time - conn->timer) >= 200) {
        conn->timer = time;

        if (!conn->connected)
            send_connect(conn, time);
        else
            net_send(conn->sock, &conn->addr, &conn->id, 4);
    }

    return 1;
}

bool conn_send(conn_t *conn, void *data, unsigned len)
{
    unsigned start, end;
    netbuf_t buf[2];

    if (((conn->seq_out + 2) & 0xFF) == conn->seq_c)
        return 0;

    start = conn->buf_offset[conn->seq_c];
    end = conn->buf_offset[conn->seq_out];
    if (((end - start) & 0xFFFF) + len >= 65536) //16bit overflow
        return 0;

    start = end;
    end = (end + len) & 0xFFFF;

    conn->buf_offset[(conn->seq_out + 1) & 0xFF] = end;

    if (start <= end) {
        memcpy(conn->buf + start, data, len);
    } else {
        memcpy(conn->buf + start, data, 65536 - start);
        memcpy(conn->buf, data + (65536 - start), end);
    }

    buf[0] = net_buf(&conn->id, 4);
    buf[1] = net_buf(data, len);
    net_sendmsg(conn->sock, &conn->addr, buf, 2);
    conn->seq_out++;
    return 1;
}

void conn_resend(conn_t *conn)
{
    size_t len, start, end;
    netbuf_t buf[3];

    start = conn->buf_offset[conn->seq_c];
    end = conn->buf_offset[conn->seq_out];
    len = (end - start) & 0xFFFF;

    conn->seq_out = conn->seq_c;
    conn->buf_offset[(conn->seq_out + 1) & 0xFF] = end;

    buf[0] = net_buf(&conn->id, 4);
    if (start <= end) {
        buf[1] = net_buf(conn->buf + start, len);
        net_sendmsg(conn->sock, &conn->addr, buf, 2);
    } else {
        buf[1] = net_buf(conn->buf + start, 65536 - start);
        buf[2] = net_buf(conn->buf, end);
        net_sendmsg(conn->sock, &conn->addr, buf, 3);
    }
    conn->seq_out++;
}
