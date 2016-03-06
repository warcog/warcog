#include "text.h"
#include "text/freetype.h"
#include "util.h"
#include "resource.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define bitmap_width 1024
#define bitmap_height 1024

//TODO: unicode support (font_t/glyph_t)
//TODO: format should support 32bit char values and >64k gylphs
//      (but my perfectly sized structs..., watch out for alignment issues if resizing)

typedef struct {
    uint16_t height, ascender;
} header_t;

typedef struct {
    uint8_t num_contour, num_kern;
    uint16_t num_point, advance, ch;

    int16_t xmin, xmax, ymin, ymax;
} glyphdata_t;

typedef struct {
    uint16_t glyph;
    int16_t dist;
} kern_t;

typedef struct {
    int16_t x, y; /* least significant bit of x -> tag */
} point_t;

enum {
    max_points = 1024,
};

static int fixed_mul(int a, unsigned b)
{
    if (a < 0)
        return -(((int64_t)-a * b + 0x8000) >> 16);
    else
        return (((int64_t)a * b + 0x8000) >> 16);
}

static int apply_size(int a, unsigned b)
{
    return ((((int64_t) a * b) + (1 << 21)) >> 22);
}

static bool render(data_t data, FT_Raster raster, FT_Bitmap *bitmap, atlas_t *atlas,
                   font_t *font, const unsigned *sizes, unsigned count)
{
    FT_Vector points[max_points];
    char tag[max_points];
    FT_Outline outline;
    FT_Raster_Params params;

    header_t *header;
    glyphdata_t *g, *glyph;
    kern_t *kern;
    uint16_t *contour;
    point_t *point, *p;
    unsigned num_glyph, num_kern, num_contour, num_point;
    unsigned i, j, k, size, kn, w, h;
    int xmin, xmax, ymin, ymax;
    uvec2 pos;

    header = dp_read(&data, sizeof(header_t));
    if (!header)
        return 0;

    num_kern = num_contour = num_point = 0;
    glyph = (void*) data.data;
    for (num_glyph = 0; !dp_empty(&data); num_glyph++) {
        g = dp_read(&data, sizeof(glyphdata_t));
        if (!g || !dp_expect(&data, g->num_kern * 4 + g->num_contour * 2 + g->num_point * 4))
            return 0;

        num_kern += g->num_kern;
        num_contour += g->num_contour;
        num_point += g->num_point;
    }

    kern = (void*) &glyph[num_glyph];
    contour = (void*) &kern[num_kern];
    point = (void*) &contour[num_contour];

    outline.points = points;
    outline.tags = tag;
    outline.flags = 0;

    params.source = &outline;
    params.flags = FT_RASTER_FLAG_AA;
    params.target = bitmap;

    for (k = 0; k < count; k++) {
        size = sizes[k];

        font[k].height = apply_size(header->height, size);
        font[k].y = apply_size(header->ascender, size);

        outline.contours = (short*) contour;
        p = point;
        kn = 0;

        for (i = 0; i < num_glyph; i++, outline.contours += g->num_contour) {
            g = &glyph[i];

            xmin = ( fixed_mul(g->xmin, size)       >> 6);
            xmax = ((fixed_mul(g->xmax, size) + 63) >> 6);
            ymin = ( fixed_mul(g->ymin, size)       >> 6);
            ymax = ((fixed_mul(g->ymax, size) + 63) >> 6);

            w = xmax - xmin;
            h = ymax - ymin;

            if (!atlas_commit(atlas, &pos, w, h)) {
                printf("out of space in glyph atlas\n");
                return 0;
            }

            for (j = 0; j < g->num_point; j++, p++) {
                tag[j] = p->x & 1;
                points[j].x = fixed_mul(p->x / 2, size) + ((pos.x - xmin) << 6);
                points[j].y = fixed_mul(p->y, size)     + ((pos.y - ymin) << 6);
            }

            outline.n_contours = g->num_contour;
            outline.n_points = g->num_point;

            //TODO unicode
            if (g->ch >= ' ' && g->ch < 127) {
                glyph_t *f = &font[k].glyph[g->ch - ' '];

                f->x = pos.x;
                f->y = bitmap_height - (pos.y + h);
                f->advance = apply_size(g->advance, size);
                f->width = w;
                f->height = h;
                f->left = -xmin;
                f->top = ymax - apply_size(header->ascender, size);

                for (j = 0; j < g->num_kern; j++) {
                    glyphdata_t *h = &glyph[kern[kn + j].glyph];
                    if (h->ch >= ' ' && h->ch < 127)
                        f->kern[h->ch - ' '] = apply_size(kern[kn + j].dist, size);
                }
            }
            kn += g->num_kern;

            ft_grays_raster.raster_render(raster, &params);
        }
    }

    return 1;
}

bool text_rasterize(font_t *font, void *buffer, unsigned height)
{
    unsigned sizes[] = {62783, 0x10000 * height / 768};
    uint64_t pool[768] = {0};

    FT_Raster raster;
    FT_Bitmap bitmap;
    atlas_t atlas;
    data_t data;
    bool res;

    if (!read_file(&data, "font", 0))
        return 0;

    atlas_init(&atlas, bitmap_width, bitmap_height);

    ft_grays_raster.raster_new(0, &raster);
    ft_grays_raster.raster_reset(raster, (void*) pool, sizeof(pool));

    bitmap.pixel_mode = FT_PIXEL_MODE_GRAY;
    bitmap.num_grays  = 256;
    bitmap.width = bitmap_width;
    bitmap.rows = bitmap_height;
    bitmap.pitch = bitmap_width;
    bitmap.buffer = buffer;

    memset(bitmap.buffer, 0, bitmap_width * bitmap_width); //TODO

    res = render(data, raster, &bitmap, &atlas, font, sizes, num_font);

    free(data.data);
    return res;
}

static char* glyph_next(const font_t *font, const char *str, int *size, const glyph_t **g, int *kern)
{
    char *res;
    uint32_t ch;
    const glyph_t *f;

    res = utf8_next(str, &ch, size);
    if (res) {
        if (ch == '\n') {
            *g = 0;
            return res;
        }

        ch -= ' ';
        if (ch >= 95)
            ch = '?' - ' ';

        f = *g;
        *g = &font->glyph[ch];
        *kern = f ? (*g)->kern[f - font->glyph] : 0;
        //if (*kern)
        //    printf("kerning: %i\n", *kern);
    }
    return res;
}

uint32_t text_width(const font_t *font, const char *str)
{
    const glyph_t *g;
    uint32_t width;
    int kern;

    g = 0;
    width = 0;
    while ((str = glyph_next(font, str, 0, &g, &kern)))
        width += kern + g->advance;

    return width;
}

uint32_t text_width2(const font_t *font, const char *str, uint32_t *res, int i)
{
    const glyph_t *g;
    uint32_t width;
    int kern, size;

    g = 0;
    width = 0;
    *res = 0;
    while ((str = glyph_next(font, str, &size, &g, &kern))) {
        width += kern + g->advance;
        if (!(i -= size))
            *res = width;
    }
    return width;
}

uint32_t text_fit(const font_t *font, const char *str, int width)
{
    const glyph_t *g;
    uint32_t res;
    int kern, size;

    g = 0;
    res = 0;
    while ((str = glyph_next(font, str, &size, &g, &kern))) {
        if ((width -= kern + g->advance) < 0)
            break;
        res += size;
    }
    return res;
}

vert2d_t* text_draw(vert2d_t *v, const font_t *font, const char *str, int x, int y, rgba color)
{
    const glyph_t *g;
    int kern;

    g = 0;
    while ((str = glyph_next(font, str, 0, &g, &kern))) {
        x += kern;
        *v++ = quad(x - g->left, y - g->top, g->width, g->height, (g->x|(g->y<<12)|(1<<30)), color);
        x += g->advance;
    }
    return v;
}

vert2d_t* text_draw_centered(vert2d_t *v, const font_t *font, const char *str, int x, int w,
                             int y, rgba color)
{
    return text_draw(v, font, str, x + (w - text_width(font, str)) / 2, y, color);
}

vert2d_t* text_draw_clip(vert2d_t *v, const font_t *font, const char *str, int x, int y,
                         int min, int max, rgba color)
{
    const glyph_t *g;
    int kern, d, h, vy, vh;

    g = 0;
    while ((str = glyph_next(font, str, 0, &g, &kern))) {
        vy = y - g->top;
        d = 0;
        if (vy < min) {
            d = min - vy;
            vy = min;
        }

        if (g->height <= d)
            goto skip;

        vh = g->height - d;

        h = max - vy;
        if (h <= 0)
            goto skip;

        if (vh > h)
            vh = h;

        x += kern;
        *v++ = quad(x - g->left, vy, g->width, vh, (g->x|(g->y<<12)|(1<<30)), color);
    skip:
        x += g->advance;
    }

    return v;
}

//TODO
uint32_t text_height(const font_t *font, const char *str, uint32_t w)
{
    const glyph_t *g;
    int kern;
    uint32_t x, y;

    g = 0;
    x = 0;
    y = font->y;
    while ((str = glyph_next(font, str, 0, &g, &kern))) {
        if (!g) { //newline
            x = 0, y += font->height;
            continue;
        }

        x += kern + g->advance;
        if (x > w)
            x = g->advance, y += font->height, g = 0;
    }

    return y;
}

vert2d_t* text_box(vert2d_t *v, const font_t *font, const char *str, int x, int y, int w, rgba color)
{
    const glyph_t *g;
    int kern, x_min, x_max;
    bool newline;

    g = 0;
    x_min = x;
    x_max = x + w;
    while ((str = glyph_next(font, str, 0, &g, &kern))) {
        if (!g) { //newline
            x = x_min, y += font->height;
            continue;
        }

        x += kern;
        newline = 0;
        if (x + g->advance > x_max)
            x = x_min, y += font->height, newline = 1;

        *v++ = quad(x - g->left, y - g->top, g->width, g->height, (g->x|(g->y<<12)|(1<<30)), color);
        x += g->advance;
        if (newline)
            g = 0;
    }

    return v;
}
