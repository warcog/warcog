#include <stdint.h>

typedef struct {
    uint16_t ip;
    uint8_t unused, next;
    uint32_t key, value;
} ipentry_t;

typedef struct {
    ipentry_t* entry[65536];
} iptable_t;

ipentry_t* ip_find(iptable_t *table, uint32_t ip); /* can fail: oom */
void ip_clear(iptable_t *table);
