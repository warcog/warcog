#ifndef MAP_H
#define MAP_H

#include <stdint.h>
#include <stdbool.h>
#include "math.h"

enum {
    map_bytes_per_tile = 6,
};

typedef struct {
    uint8_t x, y;
    uint8_t tile[2];
    uint8_t height[4];
} mapvert_t;

typedef struct {
    mapvert_t m;
    float x, y;
} mapvert2_t;

typedef struct {
    uint8_t min_h, max_h;
} maptree_t;

typedef struct {
    uint8_t size_shift;
    uint32_t nvert, size3, size;

    maptree_t *tree;
    mapvert_t *vert;
    uint8_t *tile;

    /*int , size, nvert,;
    deform_t deform[255]; */
} map_t;

typedef struct {
    uint8_t x, y, depth;
    uint16_t i;
} treesearch_t;

float map_height(const map_t *map, uint32_t x, uint32_t y);
vec3 map_intersect_tile(const map_t *map, vec3 pos, vec3 ray, uint32_t x, uint32_t y);

bool map_load(map_t *map, const void *data, uint8_t size_shift) use_result;
void map_free(map_t *map);

mapvert2_t* map_circleverts(const map_t *map, mapvert2_t *v, const mapvert2_t *max,
                            uint32_t px, uint32_t py, float r);

void map_quad_iterate(const map_t *map, void *user, bool (*fn) (void*, vec3, vec3, treesearch_t *t));

#endif


