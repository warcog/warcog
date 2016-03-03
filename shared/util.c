#include "util.h"

#ifdef WIN32
#include <windows.h>
static LARGE_INTEGER freq;
#else
#include <time.h>
#endif

bool time_init(void)
{
#ifdef WIN32
    return QueryPerformanceFrequency(&freq);
#else
    return 1;
#endif
}

uint64_t time_get(void)
{
#ifdef WIN32
    LARGE_INTEGER i;

    QueryPerformanceCounter(&i);
    return i.QuadPart;
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts))
        return 0;

    return (uint64_t) ts.tv_sec * 1000000000 + ts.tv_nsec;
#endif
}

double time_seconds(uint64_t value)
{
#ifdef WIN32
    return (double) value / freq.QuadPart;
#else
    return (double) value / 1000000000.0;
#endif

}

uint64_t time_msec(uint64_t value)
{
#ifdef WIN32
    return (value * 1000ul + (freq.QuadPart / 2)) / freq.QuadPart;
#else
    return (value + 500000ul) / 1000000ul;
#endif
}

/*bool time_msec(uint64_t value, size_t msec)
{
#ifdef WIN32
    value *= 1000ul;
    //if (__builtin_mul_overflow(value, 1000ul, &value))
    //    return 1;
    return value >= freq.QuadPart * msec;
#else
    return value >= msec * 1000000ul;
#endif
}*/

/* subtract y from *x, return true if overflow */
#define sub_of(x, y) ({ \
    bool res; \
    typeof(x) __x = x; \
    typeof(*(x)) __y = y; \
    res = *__x < __y; \
    *__x -= __y; \
    res; \
})

void* dp_read(data_t *p, size_t len)
{
    void *res;

    res = sub_of(&p->len, len) ? 0 : p->data;
    p->data += len;

    return res;
}

void* dp_next(data_t *p, size_t len)
{
    void *res;

    res = p->data;
    p->data += len;

    return res;
}

bool dp_expect(data_t *p, size_t len)
{
    return !sub_of(&p->len, len);
}

void dp_skip(data_t *p, size_t len)
{
    p->data += len;
}

bool dp_empty(data_t *p)
{
    return !p->len;
}
