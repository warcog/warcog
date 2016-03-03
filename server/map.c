#include "map.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

bool map_path(map_t *map, point *res, point a, point b, uint32_t radius, uint32_t range)
{
    (void) radius;
    (void) range;
    (void) a;
    (void) map;

    *res = b;
    return 1;
}

void map_build(map_t *map)
{
    (void) map;
}

void map_entity_init(map_t *map)
{
    (void) map;
}

static uint16_t zvalue(point p)
{
    uint8_t x, y;
    x = p.x >> 20;
    y = p.y >> 20;

    return
        (((x * 0x0101010101010101ULL & 0x8040201008040201ul) * 0x0102040810204081ul >> 49) & 0x5555) |
        (((y * 0x0101010101010101ULL & 0x8040201008040201ul) * 0x0102040810204081ul >> 48) & 0xAAAA);
}

void map_entity_add(map_t *map, point p, uint16_t id, uint16_t i)
{
    uint16_t z;

    z = zvalue(p);

    //insert
    for (; 1; map->ent[i] = map->ent[i - 1], i--) {
        if (i && map->ent[i - 1].z >= z)
            continue;

        map->ent[i].id = id;
        map->ent[i].z = z;
        return;
    }
}

#define maptree(m, level, index) (&m->tree[(1 << ((level)*2))/3 + (index)])

void map_entity_finish(map_t *map, uint16_t nentity)
{
    mtree_t *m;
    int i, j, k;

    for (i = 0; i <= map_shift; i++) {
        m = maptree(map, i, 0);
        m->start = 0;

        k = 0;

        for (j = 0; j < nentity; j++) {
            while (map->ent[j].z >= ((k + 1) << (16 - i*2))) {
                m->end = j; m++; m->start = j;
                k++;
            }
        }
        while (k + 1 < (1 << (i*2))) {
            m->end = j; m++; m->start = j;
            k++;
        }
        m->end = j;
    }
}

void aoe(map_t *map)
{

}

void map_vision_init(map_t *map, point p, uint32_t radius)
{
    (void) map;
    (void) p;
    (void) radius;
}

bool map_vision_check(map_t *map, point p)
{
    (void) map;
    (void) p;

    return 1;
}


