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
} data_t, file_t;
#define data(data, len) ((data_t){(uint8_t*) data, len})
#define data_none data(0, 0)

void* dp_read(data_t *p, size_t len);
void* dp_next(data_t *p, size_t len);
bool dp_expect(data_t *p, size_t len);
void dp_skip(data_t *p,  size_t len);
bool dp_empty(data_t *p);

/*#define sub_of(x, y) ({ \
    bool res; \
    typeof(x) __x = x; \
    typeof(*(x)) __y = y; \
    res = *__x < __y; \
    *__x -= __y; \
    res; \
})

#define dp_read(p, l) ({ \
    void *res; \
    size_t len = l; \
    res = sub_of(&(p)->len, l) ? 0 : (p)->data; \
    (p)->data += l; \
    res; \
})

#define dp_next(p, l) ({ \
    void *res; \
    res = (p)->data; \
    (p)->data += l; \
    res; \
})

#define dp_expect(p, l) ({ \
    !sub_of(&(p)->len, l); \
})

#define dp_skip(p, l) ({ \
    (p)->data += l; \
    ; \
})

#define dp_empty(p) ({ \
    !(p)->len; \
})
*/
#endif
