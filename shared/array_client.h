#ifndef ARRAY_H
#define ARRAY_H

#define array(type, id_type, max) \
struct { \
    id_type count, id[max]; \
    type data[max]; \
}

#define array_for(a, name) for (typeof((a)->data[0]) *name, *__i = 0; \
    name = &(a)->data[(a)->id[(size_t) __i]], (size_t) __i < (a)->count; \
    __i = (void*)((size_t) __i + 1))

#define array_for_all(a, name) for (typeof((a)->data[0]) *name, *__i = 0; \
    name = &(a)->data[(size_t) __i], (size_t) __i < countof((a)->data); \
    __i = (void*)((size_t) __i + 1))

#define array_keep(a, p, i) ((a)->id[i] = array_id(a, p), i + 1)
#define array_add(a, p) ((a)->id[(a)->count++] = array_id(a, p))
#define array_update(a, i) ((a)->count = (i))
#define array_id(a, p) ((p) - (a)->data)
#define array_get(a, id) (&((a)->data[id]))
#define array_reset(a) ((a)->count = 0)

#endif
