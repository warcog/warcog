#include <stdint.h>
#include <stdbool.h>
#include <net.h>

typedef struct {
    uint32_t key;
    union {
        struct {
            uint8_t value, id;
        };
        uint16_t ping;
    };
    uint8_t players, maxplayers;
    uint64_t checksum;
    char name[16], desc[32];
} gameinfo_t;

typedef struct {
    struct {
        uint8_t unused[2];
        uint16_t port;
        uint32_t ip;
    } addr;

    gameinfo_t info;
} game_t;

typedef struct {
    void *obj;
    int sock;
    addr_t addr;

    uint32_t key;

    bool connected;
    uint8_t timeout, seq_c;

    uint8_t ngame, npinged, pingid, pingtimeout;

    struct {
        uint32_t id;
        uint8_t flags, seq_in, seq_out, unused2;
    };

    uint16_t buf_offset[256];
    uint8_t buf[65536];

    uint64_t ping_time[16];
    uint8_t sort[255];
    game_t list[255];
} chat_t;

/* */
void chat_userlist_cb(chat_t *chat);
void chat_gamelist_cb(chat_t *chat);

void chat_status_cb(chat_t *chat);
void chat_name_cb(chat_t *chat, uint32_t id, const char *name, uint8_t len);
void chat_leave_cb(chat_t *chat, uint32_t id);
void chat_msg_cb(chat_t *chat, uint32_t id, const char *msg, uint8_t len);

/* */
bool chat_init(chat_t *chat, void *obj);
void chat_free(chat_t *chat);
void chat_recv(chat_t *chat);
void chat_timer(chat_t *chat);

bool chat_name(chat_t *chat, const char *name, uint8_t len);
bool chat_send(chat_t *chat, const char *msg, uint8_t len);

bool chat_gamelist_refresh(chat_t *chat);


