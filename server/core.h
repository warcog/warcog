#ifndef CORE_H
#define CORE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define distance(x) ((x) * 65536)
#define distance_max (~0)
#define speed(x) (distance(x) / FPS)
#define speedi(x) ((distance(x) + FPS/2) / FPS)
#define accel(x) (speed(x) / FPS)
#define acceli(x) ((speed(x) + FPS/2) / FPS)
#define turnrate(x) (((x) + FPS/2) / FPS)

#define hp(x) ((x) * 65536)
#define hpsec(x) ((hp(x) + FPS/2) / FPS)

#define mana(x) ((x) * 65536)
#define manasec(x) ((mana(x) + FPS/2) / FPS)

#define sec(x) ((x) * FPS)
#define msec(x) (((x) * FPS + 500) / 1000)

#define totile(x) ((x) / (65536 * 16))
#define tile(x) distance((x) * 16)
#define tiles(x,y) position((x) * 16, (y) * 16)

#define full_circle 65536
#define half_circle (full_circle / 2)

typedef struct {
    union {
        struct { uint32_t x, y; };
        uint32_t val[2];
    };
} point;

typedef struct {
    union {
        struct { int32_t x, y; };
        int32_t val[2];
    };
} delta;

typedef struct {
    union {
        struct { double x, y; };
        double val[2];
    };
} vec;

uint32_t mul(uint32_t a, double b);

point makepoint(uint32_t x, uint32_t y);
#define point(x, y) makepoint(x, y)
#define position(x, y) point(distance(x), distance(y))
point addvec(point p, vec v);
point mirror(point a, point b);
uint64_t mag2(delta d);
uint64_t dist2(point a, point b);
bool inrange(point a, point b, uint32_t range);
bool inrange_line(double *res, point a, point b, vec dir, uint32_t range);
uint16_t getangle(point a, point b);
bool facing(uint16_t theta, uint16_t angle, uint16_t max);

delta makedelta(point a, point b);
#define delta(a, b) makedelta(a, b)

vec makevec(double x, double y);
#define vec(x, y) makevec(x, y)
vec vec_angle(double angle, double distance);
vec vec_p(point a, point b);
vec vec_normp(point a, point b);
vec scale(vec v, double k);
vec add(vec a, vec b);
double dot(vec a, vec b);
double mag(vec v);
vec norm(vec v);
uint16_t angle(vec v);
vec rotate(vec v, double theta);
vec reflect(vec v, vec x);

typedef struct {
    uint8_t type, alt;
    uint16_t ent;
    point pos;
} target_t;

typedef struct {
    uint8_t id;
    uint16_t timer;
    uint8_t target_type, alt;
    union {
        uint16_t ent_id;
        point p;
    };
} order_t;

typedef struct {
    uint16_t frame;
    uint8_t id, len;
} anim_t;

typedef struct {
    int8_t value;
    uint8_t anim;
    uint16_t cast_time, anim_time;
    uint32_t range;
} cast_t;

cast_t makecast(int8_t value, uint8_t anim, uint16_t cast_time, uint16_t anim_time, uint32_t range);
#define cast(a,b,c) makecast(1, a, b, c, 0)
#define cast_wait makecast(-3, 0, 0, 0, 0)
#define cast_norange(d) makecast(-2, 0, 0, 0, d)
#define cast_noangle makecast(-1, 0, 0, 0, 0)
#define cast_cancel makecast(0, 0, 0, 0, 0)

#endif
