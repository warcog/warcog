#ifndef RESOURCE_H
#define RESOURCE_H

#include <stdbool.h>
#include <stdio.h>
#include <util.h>
#include "math.h"

enum {
    max_models = 1024,
    max_sounds = 2048,
    max_textures = 2048,
};

typedef struct {
    uint32_t offset;
    uint16_t w, h;
} texinfo_t;

typedef struct {
    FILE *fp;
    texinfo_t info[max_textures];
} texfile_t;

typedef struct {
    uint32_t nsample;
    float data[];
} sound_t;

bool read_file(data_t *data, const char *path, size_t min_size) use_result;

bool read_texture_file(texfile_t *file, const char *path) use_result;
void close_texture_file(texfile_t *file);

texinfo_t* get_texture_info(texfile_t *file, uint16_t id);
bool read_texture(texfile_t *file, const texinfo_t *info, rgba *data) use_result;
bool copy_texture(texfile_t *file, const texinfo_t *info, rgba *data, uint32_t line_size) use_result;

sound_t* read_sound(data_t data, uint16_t id);
data_t read_model(data_t data, uint16_t id);
bool load_tileset(texfile_t *file, rgba *data, const int16_t *tileset, size_t max) use_result;

typedef struct {
    unsigned count, max;
    uint16_t *data;
} idmap_t;

void idmap_init(idmap_t *map, uint16_t *data, unsigned max);
bool idmap_test(idmap_t *map, unsigned *id) use_result;
unsigned idmap_add(idmap_t *map, unsigned id);

typedef struct {
    unsigned x, line_height, w, h;
} atlas_t;

void atlas_init(atlas_t *a, unsigned width, unsigned height);
bool atlas_commit(atlas_t *a, point *p, unsigned width, unsigned height) use_result;

typedef struct {
    unsigned x, y, w, h, line_height, tile_size;
    int16_t *map;
} tatlas_t;

void tatlas_init(tatlas_t *a, unsigned width, unsigned height, unsigned tile_size, int16_t *map);
int tatlas_add(tatlas_t *a, rgba *data, texfile_t *file, unsigned id);

#endif
