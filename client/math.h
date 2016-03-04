#ifndef MATH_H
#define MATH_H

#include <stddef.h>
#include <stdint.h>

#include <math.h>
void sincosf(float, float*, float*);

typedef uint32_t rgba;
#define rgba(r,g,b,a) ((r) | ((g) << 8) | ((b) << 16) | ((a) << 24))

typedef struct {
    unsigned x, y;
} uvec2, point;
#define point(x, y) ((point){x, y})

typedef union {
    struct { int32_t x, y; };
    struct { int32_t min, max; };
    int32_t v[2];
} ivec2;
#define ivec2(x, y) ((ivec2){{x, y}})

typedef union {
    struct { int16_t x, y, z; };
} ivec3;

typedef struct {
    ivec2 pos, tex;
} ivert;

typedef union {
    struct { float x, y; };
    struct { float w, h; };
    struct { float min, max; };
    float v[2];
} vec2;
#define vec2(x, y) ((vec2){{x, y}})

typedef union {
    struct { float x, y, z; };
    vec2 xy;
    float v[3];
} vec3;
#define vec3(x, y, z) ((vec3){{x, y, z}})

typedef union {
    struct { float x, y, z, w; };
    float v[4];
    vec3 xyz;
    struct { vec2 xy; vec2 zw; };
} vec4;
#define vec4(x, y, z, w) ((vec4){{x, y, z, w}})

typedef struct {
    union {
        float m[4][4];
        vec4 v[4];
    };
} matrix4_t;

typedef struct {
    vec3 pos, x, y, z;
} ref_t;

typedef struct {
    int16_t x, y, w, h;
    uint32_t tex;
    rgba color;
} vert2d_t;

#define typeof_noconst(x) typeof(_Generic(x, \
    vec2 : vec2(0.0, 0.0), \
    vec3 : vec3(0.0, 0.0, 0.0), \
    vec4 : vec4(0.0, 0.0, 0.0, 0.0), \
    ivec2 : ivec2(0, 0) \
))

#define typeof_noptr(x) typeof(_Generic(x, \
    vec2* : vec2(0.0, 0.0), \
    vec3* : vec3(0.0, 0.0, 0.0), \
    vec4* : vec4(0.0, 0.0, 0.0, 0.0), \
    default: x \
))

#define _eval(x) _Generic(x, \
    vec2* : *(&x), \
    vec3* : *(&x), \
    vec4* : *(&x), \
    default: &x \
)

#define _res(x, y) _Generic(x, \
    vec2* : x, \
    vec3* : x, \
    vec4* : x, \
    default: y \
)

/* these macros only evaluate arguments once */
#define neg(a) ({ \
    typeof(a) __a = a; \
    typeof_noconst(__a) __x; \
    for (size_t __i = 0; __i < countof(__a.v); __i++) \
        __x.v[__i] = -__a.v[__i]; \
__x;})

#define inv(a) ({ \
    typeof(a) __a = a; \
    typeof_noconst(__a) __x; \
    for (size_t __i = 0; __i < countof(__a.v); __i++) \
        __x.v[__i] = 1.0 / __a.v[__i]; \
__x;})

#define _add(r, a, b) ({ \
    for (size_t __i = 0; __i < countof(a.v); __i++) \
        (r)->v[__i] = a.v[__i] + b.v[__i]; \
*(r);})

#define add(a, b) ({ \
    typeof(a) __a = a; \
    typeof_noptr(__a) __tmp = *_eval(__a); \
    typeof(__tmp) __b = b; \
    typeof_noconst(__tmp) __x; \
    (void) __x; \
    _add(_res(__a, &__x), __tmp, __b); \
})

#define add3(a, b, c) add(add(a, b), c)

#define sub(a, b) ({ \
    typeof(a) __a = a; \
    typeof(__a) __b = b; \
    typeof_noconst(__a) __x; \
    for (size_t __i = 0; __i < countof(__a.v); __i++) \
        __x.v[__i] = __a.v[__i] - __b.v[__i]; \
__x;})

#define dot(a, b) ({ \
    typeof(a) __a = a; \
    typeof(__a) __b = b; \
    float __x = 0.0; \
    for (size_t __i = 0; __i < countof(__a.v); __i++) \
        __x += __a.v[__i] * __b.v[__i]; \
__x;})

#define scale(a, b) ({ \
    typeof(a) __a = a; \
    float __b = b; \
    typeof_noconst(__a) __x; \
    for (size_t __i = 0; __i < countof(__a.v); __i++) \
        __x.v[__i] = __a.v[__i] * __b; \
__x;})

#define clamp(a, min, max) ({ \
    typeof(a) __val = a; \
    typeof(__val) __min = min; \
    typeof(__val) __max = max; \
    (__val < __min) ? __min : (__val > __max) ? __max : __val; \
})

#define clampv(a, min, max) ({ \
    typeof(a) __a = a; \
    typeof(__a) __b = min; \
    typeof(__a) __c = max; \
    typeof_noconst(__a) __x; \
    for (size_t __i = 0; __i < countof(__a.v); __i++) \
        __x.v[__i] = clamp(__a.v[__i], __b.v[__i], __c.v[__i]); \
__x;})

#define mag(a) ({ \
    typeof(a) __mag_a = a; \
    sqrt(dot(__mag_a, __mag_a)) \
;})

#define norm(a) ({ \
    typeof(a) __norm_a = a; \
    scale(__norm_a, 1.0 / mag(__norm_a)) \
;})

#define lerp(a, b, t) ({ \
    typeof(a) __lerp_a = a; \
    typeof(__lerp_a) __lerp_b = b; \
    float __lerp_t = t; \
    add(scale(__lerp_a, 1.0 - __lerp_t), scale(__lerp_b, __lerp_t)) \
;})

float theta(uint16_t angle);
vec3 cross(vec3 a, vec3 b);
vec3 qrot(vec3 a, vec4 q);
vec4 qmul(vec4 a, vec4 b);
vec4 nlerp(vec4 a, vec4 b, float t);
vec3 ref_transform(vec3 pos, ref_t ref);
float ray_intersect_box(vec3 pos, vec3 div, vec3 min, vec3 max);

vec2 clamp2(vec2 a, float min, float max);
vec2 rot2(vec2 a, float theta);
vec4 vec4_color(const uint8_t *c);
vec4 colorvec(uint32_t color);
vert2d_t quad(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t tex, rgba color);

#endif
