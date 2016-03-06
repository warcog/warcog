#include <stdint.h>
#include <stdbool.h>
#include <net.h>

typedef struct {
    int sock;
    addr_t addr;
    bool active, connected;
    uint8_t seq_c;

    uint16_t ckey;

    uint32_t key;
    uint64_t lastrecv, timer;

    struct {
        uint8_t id, seq_out, frame, flags;
    };

    uint16_t buf_offset[256];
    uint8_t buf[65536];
} conn_t;

bool conn_init(conn_t *conn);
void conn_connect(conn_t *conn, const addr_t *addr, uint32_t key, uint64_t time);
void conn_disconnect(conn_t *conn);
bool conn_frame(conn_t *conn, uint64_t time);
bool conn_send(conn_t *conn, void *data, unsigned len);

//
void conn_resend(conn_t *conn);
void send_connect(conn_t *conn, uint64_t time);
