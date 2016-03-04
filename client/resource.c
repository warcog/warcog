#include "resource.h"
#undef float

#include <stdlib.h>
#include <string.h>
#include <opus/opus.h>
#include "model.h"

#define memcpy_aligned(dest, src, len, align) \
    (_Static_assert(len % align == 0, "len must be multiple of align for memcpy_aligned"), \
    memcpy(__builtin_assume_aligned(dest, align), __builtin_assume_aligned(src, align), len))

#define int32(x) (*((uint32_t*)(x)))

bool read_file(data_t *file, const char *path, size_t min_size)
{
    FILE *fp;
    data_t f;
    size_t res;

    fp = fopen(path, "rb");
    if (!fp)
        return 0;

    if (fseek(fp, 0, SEEK_END) || (long int) (f.len = ftell(fp)) < (long int) min_size ||
        fseek(fp, 0, SEEK_SET) || !(f.data = malloc(f.len))) {
        fclose(fp);
        return 0;
    }

    res = fread(f.data, 1, f.len, fp);
    fclose(fp);
    if (res != f.len) {
        free(f.data);
        return 0;
    }

    *file = f;
    return 1;
}

void close_file(data_t *file)
{
    free(file->data);
}

bool read_texture_file(texfile_t *file, const char *path)
{
    texfile_t f;
    size_t res;

    f.fp = fopen(path, "rb");
    if (!f.fp)
        return 0;

    res = fread(f.info, 1, sizeof(file->info), f.fp);
    if (res != sizeof(file->info)) {
        fclose(f.fp);
        return 0;
    }

    *file = f;
    return 1;
}

void close_texture_file(texfile_t *file)
{
    fclose(file->fp);
}

texinfo_t* get_texture_info(texfile_t *file, uint16_t id)
{
    texinfo_t *info;

    if (id >= max_textures)
        return 0;

    info = &file->info[id];
    //TODO: validate offset/w/h
    return info->offset ? info : 0;
}

bool read_texture(texfile_t *file, const texinfo_t *info, rgba *data)
{
    if (fseek(file->fp, info->offset, SEEK_SET))
        return 0;

    return (fread(data, info->w * info->h * 4, 1, file->fp) == 1);
}

bool copy_texture(texfile_t *file, const texinfo_t *info, rgba *data, uint32_t line_size)
{
    int i;

    if (fseek(file->fp, info->offset, SEEK_SET))
        return 0;

    for (i = 0; i < info->h; i++) {
        if (fread(data, info->w * 4, 1, file->fp) != 1)
            return 0;
        data += line_size;
    }
    return 1;
}

static sound_t* decode(const void *data, int len)
{
    static const int block_samples = 2880;
    const uint16_t *block_size;
    OpusDecoder *st;
    sound_t *s;
    float *samples;
    int i, nblock, nsample, size;

    if ((len -= 2) < 0)
        return 0;

    block_size = data;
    nblock = *block_size++;
    data = &block_size[nblock];

    if ((len -= (2 * nblock)) < 0)
        return 0;

    nsample = nblock * block_samples;
    s = malloc(sizeof(*s) + nsample * sizeof(*s->data));
    if (!s)
        return 0;

    st = opus_decoder_create(48000, 1, 0);
    if (!st)
        goto fail_free_sound;

    s->nsample = nsample;
    samples = s->data;
    for (i = 0; i < nblock; i++) {
        size = block_size[i];

        if ((len -= size) < 0)
            goto fail_free_all;

        if (opus_decode_float(st, data, size, samples, block_samples, 0) != block_samples)
            goto fail_free_all;

        data += size;
        samples += block_samples;
    }

    if (len)
        goto fail_free_all;

    opus_decoder_destroy(st);
    return s;

fail_free_all:
    opus_decoder_destroy(st);
fail_free_sound:
    free(s);
    return 0;
}

sound_t* read_sound(data_t *file, uint16_t id)
{
    uint32_t offset, end;

    if (id >= max_sounds)
        return 0;

    offset = id ? int32(file->data + (id - 1) * 4) : max_sounds * 4;
    end = int32(file->data + id * 4);
    if (!offset || end < offset || end > file->len)
        return 0;

    return decode(file->data + offset, end - offset);
}

data_t read_model(data_t *file, uint16_t id)
{
    uint32_t offset, end;

    if (id >= max_models)
        return data_none;

    offset = id ? int32(file->data + (id - 1) * 4) : max_models * 4;
    end = int32(file->data + id * 4);
    if (!offset || end < offset || end > file->len)
        return data_none;

    return data(file->data + offset, end - offset);
}

bool load_tileset(texfile_t *file, rgba *data, const int16_t *tileset, size_t max)
{
    size_t i;
    tatlas_t atlas;

    tatlas_init(&atlas, 1024, 1024, 64, 0);
    for (i = 0; i < max && tileset[i] >= 0; i++)
        if (tatlas_add(&atlas, data, file, tileset[i]) < 0)
            return 0;

    return 1;
}

void idmap_init(idmap_t *map, int16_t *data, unsigned max)
{
    map->count = 0;
    map->max = max;
    map->data = data;

    memset(data, 0xFF, sizeof(*data) * max); //TODO
}

bool idmap_test(idmap_t *map, unsigned *id)
{
    if (*id >= map->max) {
        *id = -1;
        return 1;
    }

    if (map->data[*id] >= 0) {
        *id = map->data[*id];
        return 1;
    }

    return 0;
}

unsigned idmap_add(idmap_t *map, unsigned id)
{
    map->data[id] = map->count;
    return map->count++;
}

void atlas_init(atlas_t *a, unsigned width, unsigned height)
{
    a->x = 0;
    a->line_height = 0;
    a->w = width;
    a->h = height;
}

bool atlas_commit(atlas_t *a, point *p, unsigned width, unsigned height)
{
    if (a->x + width > a->w) {
        if (sub_of(&a->h, height))
            return 0;
        a->x = 0;
        a->line_height = height;
    } else if (a->line_height < height) {
        if (sub_of(&a->h, height - a->line_height))
            return 0;
        a->line_height = height;
    }

    *p = point(a->x, a->h);
    a->x += width;
    return 1;
}

void tatlas_init(tatlas_t *a, unsigned width, unsigned height, unsigned tile_size, int16_t *map)
{
    a->x = 0;
    a->y = 0;
    a->w = width;
    a->h = height;
    a->line_height = 0;
    a->tile_size = tile_size;
    a->map = map;

    if (map)
        memset(map, 0xFF, sizeof(*map) * max_textures); //TODO
}

int tatlas_add(tatlas_t *a, rgba *data, texfile_t *file, unsigned id)
{
    const texinfo_t *info;
    unsigned res;
    bool ignore;

    if (id >= max_textures)
        return -1;

    if (a->map && a->map[id] >= 0)
        return a->map[id];

    info = get_texture_info(file, id);
    if (!info || (info->w % a->tile_size) || (info->h % a->tile_size))
        return -1;

    if (a->x + info->w > a->w) {
        if (a->y + a->line_height + info->h > a->h)
            return -1;

        a->y += a->line_height;
        a->line_height = info->h;
        a->x = 0;
    } else if (a->line_height < info->h) {
        if (a->y + info->h > a->h)
            return -1;
        a->line_height = info->h;
    }

    res = (((a->w * a->y) / a->tile_size + a->x) / a->tile_size);

    ignore = copy_texture(file, info, &data[a->y * a->w + a->x], a->w);
    (void) ignore;

    if (a->map)
        a->map[id] = res;

    a->x += info->w;
    return res;
}
