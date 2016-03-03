#include "map.h"
#include <stdlib.h>
#include <string.h>

#define max_size 8

static int morton(uint8_t x, uint8_t y)
{
    return
        (((x * 0x0101010101010101ULL & 0x8040201008040201ul) * 0x0102040810204081ul >> 49) & 0x5555) |
        (((y * 0x0101010101010101ULL & 0x8040201008040201ul) * 0x0102040810204081ul >> 48) & 0xAAAA);
}

static uint8_t max(uint8_t a, uint8_t b)
{
    return (a > b) ? a : b;
}

static uint8_t min(uint8_t a, uint8_t b)
{
    return (a < b) ? a : b;
}

static const mapvert_t* map_vertat(const map_t *map, uint32_t x, uint32_t y)
{
    x >>= 20;
    y >>= 20;
    if ((x >> map->size_shift) || (y >> map->size_shift))
        return 0;

    return &map->vert[morton(x, y)];
}

static float tilecoord(uint32_t a)
{
    return (float) (a & 0xFFFFF) / (float) (1 << 20);
}

float map_height(const map_t *map, uint32_t px, uint32_t py)
{
    const mapvert_t *v;
    float x, y, res;

    v = map_vertat(map, px, py);
    if (!v)
        return 0.0;

    /* code for 2 different triangle splits */

    x = 1.0 - tilecoord(px);
    y = tilecoord(py);

    res = v->height[1];
    if (y < x)
        res += x * ((int) v->height[0] - v->height[1]) + y * ((int) v->height[2] - v->height[0]);
    else
        res += x * ((int) v->height[2] - v->height[3]) + y * ((int) v->height[3] - v->height[1]);

    /*x = tilecoord(p.x);
    y = tilecoord(p.y);

    res = v->height[0];
    if (y < x)
        res += x * ((int) v->height[1] - v->height[0]) + y * ((int) v->height[3] - v->height[1]);
    else
        res += x * ((int) v->height[3] - v->height[2]) + y * ((int) v->height[2] - v->height[0]);
    */

    return res;
}

//TODO review
vec3 map_intersect_tile(const map_t *map, vec3 pos, vec3 ray, uint32_t x, uint32_t y)
{
    const mapvert_t *t;
    vec3 u, v, p, n;
    float d, xr, yr;

    t = &map->vert[morton(x, y)];
    p = sub(vec3((x + 1) * 16, y * 16, t->height[1]), pos);

    u = vec3(0, 16, (int) t->height[2] - t->height[0]);
    v = vec3(16, 0, (int) t->height[1] - t->height[0]);

    n = cross(u, v);
    d = dot(p, n) / dot(ray, n);
    xr = d * ray.x - p.x + 16.0;
    yr = d * ray.y - p.y;

    if (xr >= 0.0 && yr >= 0.0 && yr + xr <= 16.0)
        return vec3(x * 16 + xr, y * 16 + yr, d);

    u = vec3(-16, 0, (int) t->height[2] - t->height[3]);
    v = vec3(0, -16, (int) t->height[1] - t->height[3]);

    n = cross(u, v);
    d = dot(p, n) / dot(ray, n);
    xr = d * ray.x - p.x + 16.0;
    yr = d * ray.y - p.y;

    if (xr <= 16.0 && yr <= 16.0 && yr + xr >= 16.0)
        return vec3(x * 16 + xr, y * 16 + yr, d);

    return vec3(0.0, 0.0, INFINITY);
}

bool map_load(map_t *map, const void *data, uint8_t size_shift)
{
//TODO add cliffs back
    uint32_t i, j, k, x, y, m, size;
    maptree_t *mt, *parent, *base, *root;
    mapvert_t *v;
    uint8_t *p;

    if (size_shift > max_size)
        return 0;

    size = 1 << size_shift;
    k = size << size_shift;

    map->size_shift = size_shift;
    map->nvert= k;
    map->size3 = k / 3;
    map->size = size;
    map->vert = malloc(k * (sizeof(*map->vert) + 2) + (k + map->size3) * sizeof(*map->tree));
    if (!map->vert)
        return 0;

    map->tile = (void*) &map->vert[k];
    map->tree = (void*) &map->tile[k * 2];

    p = map->tile;
    for (y = 0; y < size; y++) {
        for (x = 0; x < size; x++) {
            m = morton(x, y);
            v = &map->vert[m];
            mt = &map->tree[map->size3 + m];

            v->x = x; v->y = y;
            memcpy(p, data, 2); p += 2;
            memcpy(v->tile, data, 6); data += 6;

            mt->min_h = min(min(v->height[0], v->height[1]), min(v->height[2], v->height[3]));
            mt->max_h = max(max(v->height[0], v->height[1]), max(v->height[2], v->height[3]));
        }
    }

    k = size_shift;
    root = &map->tree[map->size3];
    while (k--) {
        base = root;
        j = (1 << (k * 2));
        root -= j;

        for (i = 0; i < j; i++) {
            parent = &root[i];
            mt = &base[i * 4];

            parent->min_h = min(min(mt[0].min_h, mt[1].min_h), min(mt[2].min_h, mt[3].min_h));
            parent->max_h = max(max(mt[0].max_h, mt[1].max_h), max(mt[2].max_h, mt[3].max_h));
        }
    }

    return 1;
}

void map_free(map_t *map)
{
    free(map->vert);
}

mapvert2_t* map_circleverts(const map_t *map, mapvert2_t *v, const mapvert2_t *max,
                            uint32_t px, uint32_t py, float r)
{
    int minx, miny, maxx, maxy, x, y;
    uint8_t radius;
    vec2 o;

    radius = r > 255.0 ? 255 : (r + 0.5);
    o = vec2(px / 65536.0, py / 65536.0);

    r += 1.0;
    minx = (int) (o.x - r) / 16;
    maxx = (int) (o.x + r) / 16;

    miny = (int) (o.y - r) / 16;
    maxy = (int) (o.y + r) / 16;

    if (minx < 0)
        minx = 0;

    if (miny < 0)
        miny = 0;

    if (maxx > (int) map->size - 1)
        maxx = map->size - 1;

    if (maxy > (int) map->size - 1)
        maxy = map->size - 1;

    for (y = miny; y <= maxy; y++) {
        for (x = minx; x <= maxx && v != max; x++) {
            v->m = map->vert[morton(x, y)];
            v->m.tile[0] = radius;
            v->m.tile[1] = 0;
            v->x = (x * 16) - o.x;
            v->y = (y * 16) - o.y;
            v++;
        }
    }

    return v;
}

//TODO ?
void map_quad_iterate(const map_t *map, void *user, bool (*fn) (void*, vec3, vec3, treesearch_t *t))
{
    treesearch_t tree[map->size_shift * 3 + 1], *t;
    maptree_t *mt;
    vec3 min, max;
    uint32_t size;
    bool res;

    t = tree;
    t->x = 0;
    t->y = 0;
    t->i = 0;
    t->depth = 0;

    do {
        mt = &map->tree[((1 << (t->depth * 2)) / 3) + t->i];
        size = (1 << (map->size_shift - t->depth));

        min = vec3(t->x * 16, t->y * 16, mt->min_h);
        max = vec3((t->x + size) * 16, (t->y + size) * 16, mt->max_h);

        res = fn(user, min, max, t);
        if (!res) {
            t--;
        }
        /*if (test_outside()) {
            t--;
        } else if (test_complete() || t->depth == map->size_shift) {
            do_complete(size, t->x, t->y, t->i);
            t--;
        }*/ else {
            size /= 2;
            t->i *= 4;
            t->depth++;

            t[1].x = t->x + size;
            t[1].y = t->y;
            t[1].i = t->i + 1;
            t[1].depth = t->depth;

            t[2].x = t->x;
            t[2].y = t->y + size;
            t[2].i = t->i + 2;
            t[2].depth = t->depth;

            t[3].x = t->x + size;
            t[3].y = t->y + size;
            t[3].i = t->i + 3;
            t[3].depth = t->depth;

            t += 3;
        }
    } while (t >= tree);
}
