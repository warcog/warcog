#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

bool time_init(void);
uint64_t time_get(void); /* get current tick count */
double time_seconds(uint64_t value); /* ticks->seconds */
uint64_t time_msec(uint64_t value); /* ticks->msec */

typedef struct {
    uint8_t *data;
    size_t len;
} data_t;
#define data(data, len) ((data_t){(uint8_t*) data, len})
#define data_none data(0, 0)

/* subtract y from *x, return true if overflow */
#define sub_of(x, y) ({ \
    bool res; \
    typeof(x) __x = x; \
    typeof(*(x)) __y = y; \
    res = *__x < __y; \
    *__x -= __y; \
    res; \
})

void* dp_read(data_t *p, size_t len); /* advance and reduce size */
void* dp_next(data_t *p, size_t len); /* advance */
bool dp_expect(data_t *p, size_t len); /* reduce size */
bool dp_empty(data_t *p);

uint8_t* write16(uint8_t *p, uint16_t val);

#endif
