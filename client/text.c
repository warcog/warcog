#include "text.h"
#include "text/freetype.h"
#include "util.h"
#include "resource.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

static int fixed_mul(int a, int b)
{
    if (a < 0)
        return -(((int64_t)-a * b + 0x8000) >> 16);
    else
        return (((int64_t)a * b + 0x8000) >> 16);
}

static int fixed_mul2(int a, int b)
{
    return ((((int64_t) a * b) + (1 << 21)) >> 22);
}

#define bitmap_width 1024
#define bitmap_height 1024

#define sublen(x, len) (__builtin_sub_overflow(*(x), (typeof(*(x))) (len), x))
#define next(p, x, len) (sublen(x, len) ? 0 : ({void *__tmp = *(p); *(p) += (len); __tmp;}))

typedef struct {
    uint16_t height, ascender;
} header_t;

typedef struct {
    uint8_t num_contour, num_kern;
    uint16_t num_point, advance, ch;

    int16_t xmin, xmax, ymin, ymax;
} xglyph_t;

typedef struct {
    uint16_t glyph;
    int16_t dist;
} kern_t;

typedef struct {
    int16_t x, y;
} point_t;

bool text_rasterize(font_t *font, void *data, const uint32_t *sizes, uint8_t count)
{
    (void) font;
    (void) sizes;
    (void) count;

    file_t file;
    uint8_t *p;

    header_t *header;
    xglyph_t *g, *glyph;
    kern_t *kern;
    uint16_t *contour;
    point_t *point;

    int num_glyph, num_kern, num_contour, num_point;

    if (!read_file(&file, "font", 0))
        return 0;
    p = file.data;

    header = next(&p, &file.len, sizeof(*header));
    if (!header)
        goto error;

    num_glyph = num_kern = num_contour = num_point = 0;
    glyph = (void*) p;
    while (file.len) {
        g = next(&p, &file.len, sizeof(*g));

        if (sublen(&file.len, g->num_kern * 4 + g->num_contour * 2 + g->num_point * 4))
            goto error;

        num_glyph++;
        num_kern += g->num_kern;
        num_contour += g->num_contour;
        num_point += g->num_point;
    }
    kern = (void*) p;
    contour = (void*) &kern[num_kern];
    point = (void*) &contour[num_contour];

    printf("%u %u %u %u\n", num_glyph, num_kern, num_contour, num_point);

    {
    FT_Raster r;
    uint64_t pool[768] = {0};
    FT_Outline outline;
    FT_Raster_Params params;
    FT_Bitmap bitmap;
    char tag[256*4];
    FT_Vector points[256*4];

    uint32_t size;
    int i, j, k, kn;
    point_t *p;

    int x, y, w, h, lineheight;
    int xmin, xmax, ymin, ymax;

    ft_grays_raster.raster_new(NULL, &r);
    ft_grays_raster.raster_reset(r, (void*) pool, sizeof(pool));

    outline.points = points;
    outline.tags = tag;
    outline.flags = 0;

    params.source = &outline;
    params.flags = FT_RASTER_FLAG_AA;

    bitmap.pixel_mode = FT_PIXEL_MODE_GRAY;
    bitmap.num_grays  = 256;
    bitmap.width = bitmap_width;
    bitmap.rows = bitmap_height;
    bitmap.pitch = bitmap_width;
    bitmap.buffer = data;

    memset(bitmap.buffer, 0, bitmap_width * bitmap_width);

    x = y = lineheight = 0;
    for (k = 0; k < count; k++) {
        size = sizes[k];

        font[k].height = fixed_mul2(header->height, size);
        font[k].y = fixed_mul2(header->ascender, size);

        outline.contours = (short*) contour;
        p = point;

        kn = 0;

        for (i = 0; i < num_glyph; i++) {
            g = &glyph[i];

            xmin = ( fixed_mul(g->xmin, size)       >> 6);
            xmax = ((fixed_mul(g->xmax, size) + 63) >> 6);
            ymin = ( fixed_mul(g->ymin, size)       >> 6);
            ymax = ((fixed_mul(g->ymax, size) + 63) >> 6);

            w = xmax - xmin;
            h = ymax - ymin;

            //printf("%u %u\n", g->xmin, g->xmax);

            if (x + w >= 1024) {
                y += lineheight;
                lineheight = 0;
                x = 0;
            }

            if (lineheight < h)
                lineheight = h;

            for (j = 0; j < g->num_point; j++, p++) {
                tag[j] = p->x & 1;
                points[j].x = fixed_mul(p->x / 2, size) + ((x - xmin) << 6);
                points[j].y = fixed_mul(p->y, size)     + ((y - ymin) << 6);
            }

            outline.n_contours = g->num_contour;
            outline.n_points = g->num_point;

            if (g->ch >= ' ' && g->ch < 127) {
                glyph_t *f = &font[k].glyph[g->ch - ' '];

                f->x = x;
                f->y = bitmap_height - (y + h);
                f->advance = fixed_mul2(g->advance, size);
                f->width = w;
                f->height = h;
                f->left = -xmin;
                f->top = ymax - fixed_mul2(header->ascender, size);

                //f->kern_start = kn;
                //f->kern_num = g->num_kern;

                for (j = 0; j < g->num_kern; j++) {
                    xglyph_t *h = &glyph[kern[kn + j].glyph];
                    if (h->ch >= ' ' && h->ch < 127)
                        f->kern[h->ch - ' '] = fixed_mul2(kern[kn + j].dist, size);
                }
            }
            kn += g->num_kern;

            //if (g->ch >= ' ' && g->ch < 127) {
            //    int8_t *kn = &font[k].kerning[g->ch - ' '][0];
            //}

            params.target = &bitmap;
            ft_grays_raster.raster_render(r, &params);

            outline.contours += g->num_contour;
            x += w;
        }
    }
    }

error:
    close_file(&file);




/*
    const fontdata_t *fd;
    const point_t *p;
    const uint8_t *contours;

    fglyph_t f;
    int i, j, k;


    int x, y, lineheight;

    short contour[16];
    char tag[256];
    FT_Vector point[256], point_trans[256];
    FT_Outline outline;
    FT_Raster_Params params;
    FT_Bitmap bitmap;
    int xmin, xmax, ymin, ymax;
    int txmin, txmax, tymin, tymax;
    int width, height, size;
    glyph_t *g;

    ft_grays_raster.raster_new(NULL, &r);
    ft_grays_raster.raster_reset(r, (void*)pool, sizeof(pool));

    outline.points = point_trans;
    outline.tags = tag;
    outline.contours = contour;
    outline.flags = 0;

    bitmap.pixel_mode = FT_PIXEL_MODE_GRAY;
    bitmap.num_grays  = 256;
    bitmap.width = bitmap_width;
    bitmap.rows = bitmap_height;
    bitmap.pitch = bitmap_width;

    bitmap.buffer = malloc(bitmap_width * bitmap_width);
    if (!bitmap.buffer)
        return 0;

    memset(bitmap.buffer, 0, bitmap_width * bitmap_width);

    params.source = &outline;
    params.flags = FT_RASTER_FLAG_AA;

    x = 0;
    y = 0;
    lineheight = 0;

    fd = &fontdata[0];

    for (k = 0; k < count; k++) {
        contours = fontdata_contours;
        p = fontdata_points;

        font[k].height = fixed_mul2(fd->height, sizes[k]);
        font[k].y = fixed_mul2(fd->y, sizes[k]);
        font[k].spacewidth = fixed_mul2(fd->spacewidth, sizes[k]);

        for (i = 0; i < 94; i++) {
            f = fd->glyph[i];

            for (j = 0; j < f.ncontour; j++)
                contour[j] = *contours++;

            xmin = xmax = ymin = ymax = 0;
            for (j = 0; j < f.npoint; j++, p++) {
                tag[j] = p->x & 1;
                point[j].x = p->x / 2;
                if (xmin > point[j].x)
                    xmin = point[j].x;

                if (xmax < point[j].x)
                    xmax = point[j].x;

                point[j].y = p->y;
                if (ymin > point[j].y)
                    ymin = point[j].y;

                if (ymax < point[j].y)
                    ymax = point[j].y;
            }

            outline.n_contours = f.ncontour;
            outline.n_points = f.npoint;

            size = sizes[k];
            g = &font[k].glyph[i];

            txmin = (fixed_mul(xmin, size) >> 6);
            txmax = ((fixed_mul(xmax, size) + 63) >> 6);
            tymin = (fixed_mul(ymin, size) >> 6);
            tymax = ((fixed_mul(ymax, size) + 63) >> 6);

            width = txmax - txmin;
            height = tymax - tymin;

            if (x + width >= 1024) {
                y += lineheight;
                lineheight = 0;
                x = 0;
            }

            if (lineheight < height)
                lineheight = height;

            g->x = x;
            g->y = bitmap_height - (y + height) ;
            g->advance = fixed_mul2(f.advance, size);
            g->width = width;
            g->height = height;
            g->left = txmin;
            g->top = tymax;

            for (j = 0; j < f.npoint; j++) {
                point_trans[j].x = fixed_mul(point[j].x, size) + ((x - txmin) << 6);
                point_trans[j].y = fixed_mul(point[j].y, size) + ((y - tymin) << 6);
            }

            x += width;

            params.target = &bitmap;
            ft_grays_raster.raster_render(r, &params);
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, bitmap_width, bitmap_height, 0, GL_RED, GL_UNSIGNED_BYTE,
                 bitmap.buffer);
    free(bitmap.buffer);*/
    return 1;
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
