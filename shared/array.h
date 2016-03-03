#ifndef ARRAY_H
#define ARRAY_H

#define array(type, id_type, max) \
struct { \
    id_type count, id[max]; \
    id_type nfree, free[max]; \
    type data[max]; \
}

#define array_init(a) { \
    (a)->count = 0; \
    for (typeof((a)->count) i = 0; i < array_max(a); i++) \
        (a)->free[i] = array_max(a) - (i + 1); \
    (a)->nfree = array_max(a); \
}

#define array_for(a, name) for (typeof((a)->data[0]) *name, *__i = 0; \
    name = &(a)->data[(a)->id[(size_t) __i]], (size_t) __i < (a)->count; \
    __i = (void*)((size_t) __i + 1))
#define array_for_const(a, name) for (const typeof((a)->data[0]) *name, *__i = 0; \
    name = &(a)->data[(a)->id[(size_t) __i]], (size_t) __i < (a)->count; \
    __i = (void*)((size_t) __i + 1))

#define array_keep(a, p, i) ((a)->id[i] = array_id(a, p), i + 1)
#define array_remove(a, p) ((a)->free[(a)->nfree++] = array_id(a, p))
#define array_update(a, i) ((a)->count = (i))
#define array_new(a, pid) ((a)->nfree ? \
    &((a)->data[*(pid) = (a)->id[(a)->count++] = (a)->free[--(a)->nfree]]) : 0)
#define array_id(a, p) ((p) - (a)->data)
#define array_max(a) (sizeof((a)->data) / sizeof((a)->data[0]))
#define array_get(a, id) (&((a)->data[id]))

#define array_new2(a) ((a)->nfree ? \
    ((a)->nfree--, &((a)->data[(a)->id[array_max(a) - 1 - (a)->nfree] = (a)->free[(a)->nfree]])) : 0)
#define array_update2(a) ((a)->count = array_max(a) - (a)->nfree)

//
#define sarray(type, id_type, max) \
struct { \
    id_type val, scount; \
    type sdata[max]; \
}

//init

#define sarray_for(a, name) for (typeof((a)->sdata[0]) *name, *__i = 0; \
    name = &(a)->sdata[(size_t) __i], (size_t) __i < (a)->scount; \
    __i = (void*)((size_t) __i + 1))

#define sarray_keep(a, p, i) ((a)->sdata[i] = *(p), i + 1)
#define sarray_update(a, i) ((a)->scount = (i))
#define sarray_new(a) ((a)->scount < sarray_max(a) ? &((a)->sdata[(a)->scount++]) : 0)
#define sarray_id(a, p) ((p) - (a)->sdata)
#define sarray_max(a) (sizeof((a)->sdata) / sizeof((a)->sdata[0]))
//#define sarray(a, id) (&((a)->sdata[id]))


#define carray_for(a, name, count) for (typeof(*(a)) *name, *__i = 0; \
    name = &(a)[(size_t) __i], (size_t) __i < (count); \
    __i = (void*)((size_t) __i + 1))

#endif
