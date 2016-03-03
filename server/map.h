#include <stdint.h>
#include <stdbool.h>
#include "core.h"
#include "data.h"

_Static_assert(map_size <= 256, "map too large for data types");

enum {
    map_w = map_size,
    map_h = map_size,
    map_file_size = map_w * map_h * 5,
};

typedef struct {
    int32_t id;
    //int16_t id;
    //uint8_t unused[2];
    union {
        struct { uint8_t x, y; };
        uint8_t val[2];
    };
    uint8_t dir, len;
} edge_t;

typedef struct {
    uint8_t x, y, s, t;
    uint16_t area, next_count; //
    edge_t *next;

    /* search vars */
    edge_t prev;
    bool closed;
    uint32_t d, prio;

    /* smoothing vars */
} node_t;

typedef struct {
    uint16_t id, z;
} mentity_t;

typedef struct {
    uint16_t start, end;
} mtree_t;

typedef struct {
    uint8_t height[map_h][map_w][4];
    bool path[map_h][map_w], buf[map_h][map_w];
    int16_t id[map_h][map_w]; /* tile -> pathing node */

    mentity_t ent[65535];
    mtree_t tree[(map_size*map_size*4)/3];
    //tree_t trees[map_ntree];


    /* pathfinding */
    uint32_t node_count, pq_count;
    node_t node[(map_w * map_h + 1) / 2];
    edge_t edge[(map_w * map_h) * 2];//TODO not optimal value

    /* vision */
} map_t;


bool map_path(map_t *map, point *res, point a, point b, uint32_t radius, uint32_t range);

void map_entity_init(map_t *map);
void map_entity_add(map_t *map, point p, uint16_t id, uint16_t i);
void map_entity_finish(map_t *map, uint16_t nentity);

void map_vision_init(map_t *map, point p, uint32_t radius);
bool map_vision_check(map_t *map, point p);

void map_build(map_t *map);
