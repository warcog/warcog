#include "core.h"
#include <math.h>

uint32_t mul(uint32_t a, double b)
{
    return ((double) a * b + 0.5);
}

point makepoint(uint32_t x, uint32_t y)
{
    point res;

    res.x = x;
    res.y = y;
    return res;
}

point addvec(point p, vec v)
{
    return point(p.x + (int32_t) round(v.x), p.y + (int32_t) round(v.y));
}

point mirror(point a, point b)
{
    return point(a.x - (b.x - a.x), a.y - (b.y - a.y));
}

uint64_t mag2(delta d)
{
    return ((int64_t) d.x * d.x) + ((int64_t) d.y * d.y);
}

uint64_t dist2(point a, point b)
{
    return mag2(delta(a, b));
}

bool inrange(point a, point b, uint32_t range)
{
    return (dist2(a, b) <= (uint64_t)range * range);
}

bool inrange_line(double *res, point a, point b, vec dir, uint32_t range)
{
    vec v, n;
    double d;
    uint32_t r;

    v = vec_p(a, b);
    d = dot(v, dir);
    if (d < 0.0)
        return 0;

    n = vec(dir.y, -dir.x);
    r = fabs(dot(v, n)) + 0.5;
    if (r > range)
        return 0;

    *res = d;
    return 1;
}

uint16_t getangle(point a, point b)
{
    int32_t dx, dy;

    dx = b.x - a.x;
    dy = b.y - a.y;

    return (int)round(atan2(dy, dx) * full_circle / (2.0 * M_PI));
}

bool facing(uint16_t theta, uint16_t angle, uint16_t max)
{
    return (abs((int16_t)(angle - theta)) < max);
}

delta makedelta(point a, point b)
{
    delta d;

    d.x = b.x - a.x;
    d.y = b.y - a.y;

    return d;
}

vec makevec(double x, double y)
{
    vec res;

    res.x = x;
    res.y = y;
    return res;
}

vec vec_angle(double angle, double distance)
{
    angle = angle * M_PI * 2.0 / full_circle;
    return vec(cos(angle) * distance, sin(angle) * distance);
}

vec vec_p(point a, point b)
{
    int32_t dx, dy;

    dx = b.x - a.x;
    dy = b.y - a.y;

    return vec(dx, dy);
}

vec norm(vec v)
{
    float m = mag(v);
    if (m == 0.0)
        return vec(0.0, 1.0);
    return scale(v, 1.0 / m);
}

uint16_t angle(vec v)
{
    return (int) round(atan2(v.y, v.x) * full_circle / (2.0 * M_PI));
}

vec vec_normp(point a, point b)
{
    return norm(vec_p(a, b));
}

vec scale(vec v, double k)
{
    return vec(v.x * k, v.y * k);
}

vec add(vec a, vec b)
{
    return vec(a.x + b.x, a.y + b.y);
}

double dot(vec a, vec b)
{
    return a.x * b.x + a.y * b.y;
}

double mag(vec v)
{
    return sqrt(dot(v, v));
}

vec rotate(vec v, double theta)
{
    double cost, sint;
    theta *= M_PI / 180.0;
    cost = cos(theta);
    sint = sin(theta);

    return vec(v.x * cost - v.y * sint, v.x * sint + v.y * cost);
}

vec reflect(vec v, vec x)
{
    vec n;

    n = norm(x);
    return add(v, scale(n, -2.0 * dot(v, n)));
}

cast_t makecast(int8_t value, uint8_t anim, uint16_t cast_time, uint16_t anim_time, uint32_t range)
{
    cast_t res;

    res.value = value;
    res.anim = anim;
    res.cast_time = cast_time;
    res.anim_time = anim_time;
    res.range = range;

    return res;
}
