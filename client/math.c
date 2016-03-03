#include "math.h"
#include <stdbool.h>

float theta(uint16_t angle)
{
    return (float) angle * (M_PI / 32768.0);
}

vec3 cross(vec3 a, vec3 b)
{
    return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

vec3 qrot(vec3 a, vec4 q)
{
    return add(a, scale(cross(q.xyz, add(cross(q.xyz, a), scale(a, q.w))), 2.0));
}

vec4 qmul(vec4 a, vec4 b)
{
    return vec4(
        b.w * a.x + b.x * a.w + b.y * a.z - b.z * a.y,
        b.w * a.y - b.x * a.z + b.y * a.w + b.z * a.x,
        b.w * a.z + b.x * a.y - b.y * a.x + b.z * a.w,
        b.w * a.w - b.x * a.x - b.y * a.y - b.z * a.z
    );
}

vec4 nlerp(vec4 a, vec4 b, float t)
{
    return norm((dot(a, b) > 0.0) ? lerp(a, b, t) : lerp(a, neg(b), t));
}

vec3 ref_transform(vec3 pos, ref_t ref)
{
    return vec3(dot(pos, ref.x), dot(pos, ref.y), dot(pos, ref.z));
}

/* TODO: cleanup, return "distance" */
float ray_intersect_box(vec3 pos, vec3 div, vec3 min, vec3 max)
{
    float t[2], ty[2], tz[2];
    bool i;

    i = (div.x < 0.0);
    t[i] = (min.x - pos.x) * div.x;
    t[!i] = (max.x - pos.x) * div.x;

    if (div.x == INFINITY) {
        t[0] = (min.x > pos.x) ? INFINITY : -INFINITY;
        t[1] = (max.x > pos.x) ? INFINITY : -INFINITY;
    }

    i = (div.y < 0.0);
    ty[i] = (min.y - pos.y) * div.y;
    ty[!i] = (max.y - pos.y) * div.y;

    if (div.y == INFINITY) {
        ty[0] = (min.y > pos.y) ? INFINITY : -INFINITY;
        ty[1] = (max.y > pos.y) ? INFINITY : -INFINITY;
    }

    if ((t[0] > ty[1]) || (ty[0] > t[1]))
        return INFINITY;

    //res = fmin(t[0], ty[0]);

    if (ty[0] > t[0])
        t[0] = ty[0];
    if (ty[1] < t[1])
        t[1] = ty[1];

    i = (div.z < 0.0);
    tz[i] = (min.z - pos.z) * div.z;
    tz[!i] = (max.z - pos.z) * div.z;

    if (div.z == INFINITY) {
        tz[0] = (min.z > pos.z) ? INFINITY : -INFINITY;
        tz[1] = (max.z > pos.z) ? INFINITY : -INFINITY;
    }

    if ((t[0] > tz[1]) || (tz[0] > t[1]))
        return INFINITY;

    //res = fmin(res, tz[0]);

    if (tz[0] > t[0])
        t[0] = tz[0];
    if (tz[1] < t[1])
        t[1] = tz[1];

    if (t[1] < 0.0)
        return INFINITY;

    return 0.0;
}

//TODO: cleanup below

static float clampf(float x, float min, float max)
{
    if (x < min)
        x = min;
    else if (x > max)
        x = max;
    return x;
}

vec2 clamp2(vec2 a, float min, float max)
{
    return vec2(clampf(a.x, min, max), clampf(a.y, min, max));
}

vec2 rot2(vec2 a, float theta)
{
    float sin, cos;

    sincosf(theta, &sin, &cos);
    return vec2(a.x * cos - a.y * sin, a.y * cos + a.x * sin);
}

vec4 vec4_color(const uint8_t *c)
{
    return vec4(c[0] / 255.0, c[1] / 255.0, c[2] / 255.0, c[3] / 255.0);
}

vec4 colorvec(uint32_t color)
{
    union {
        struct { uint8_t r, g, b, a;};
        uint32_t color;
    } d;
    d.color = color;

    return scale(vec4(d.r, d.g, d.b, d.a), 1.0 / 255.0);
}

vert2d_t quad(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t tex, rgba color)
{
    vert2d_t v = {x, y, w, h,
    //0, h, w, 0,
    tex, color};
    return v;
}
