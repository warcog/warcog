#ifndef RESOURCE_H
#define RESOURCE_H

#include <stdbool.h>
#include <stdio.h>
#include <util.h>
#include "math.h"
#include "model.h"

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

bool read_file(file_t *file, const char *path, size_t min_size) use_result;
void close_file(file_t *file);

bool read_texture_file(texfile_t *file, const char *path) use_result;
void close_texture_file(texfile_t *file);

texinfo_t* get_texture_info(texfile_t *file, uint16_t id) use_result;
bool read_texture(texfile_t *file, const texinfo_t *info, rgba *data) use_result;
bool copy_texture(texfile_t *file, const texinfo_t *info, rgba *data, uint32_t line_size) use_result;

sound_t* read_sound(file_t *file, uint16_t id) use_result;
data_t read_model(file_t *file, uint16_t id) use_result;
bool load_tileset(texfile_t *file, rgba *data, const int16_t *tileset, size_t max) use_result;

typedef struct {
    uint32_t count, max;
    int16_t *data;
} idmap_t;

void idmap_init(idmap_t *map, int16_t *data, size_t max);
bool idmap_test(idmap_t *map, uint16_t *id) use_result;
size_t idmap_add(idmap_t *map, uint16_t id);

typedef struct {
    uint32_t x, y;
    uint32_t w, h;
    rgba *data;

    uint32_t lh;
} atlas_t;

void atlas_init(atlas_t *a, size_t width, size_t height, rgba *data);
bool atlas_add(atlas_t *a, texfile_t *file, uint16_t id) use_result;
int16_t atlas_add_mapped(atlas_t *a, texfile_t *file, uint16_t id, int16_t *map);

#endif
