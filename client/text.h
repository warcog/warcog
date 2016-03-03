#ifndef TEXT_H
#define TEXT_H

#include <stdint.h>
#include <stdbool.h>
#include "math.h"

typedef struct {
    uint16_t x, y;

    uint8_t width, height;
    int8_t left, top;

    //uint16_t kern_start;
    //uint8_t kern_num;
    uint8_t advance;

    int8_t kern[95];
} glyph_t;

typedef struct {
    uint8_t y, height;
    glyph_t glyph[95];
    //int8_t kerning[95][95];
} font_t;

bool text_rasterize(font_t *font, void *data, const uint32_t *sizes, uint8_t count);

uint32_t text_width(const font_t *font, const char *str);
uint32_t text_width2(const font_t *font, const char *str, uint32_t *res, int i);
uint32_t text_fit(const font_t *font, const char *str, int w);

vert2d_t* text_draw(vert2d_t *v, const font_t *font, const char *str, int x, int y, rgba color);
vert2d_t* text_draw_centered(vert2d_t *v, const font_t *font, const char *str, int x, int w,
                             int y, rgba color);
vert2d_t* text_draw_clip(vert2d_t *v, const font_t *font, const char *str, int x, int y,
                         int min, int max, rgba color);

uint32_t text_height(const font_t *font, const char *str, uint32_t w);
vert2d_t* text_box(vert2d_t *v, const font_t *font, const char *str, int x, int y, int w, rgba color);


/*int loadfont(font_t *font, const int *sizes, int count, const void *data, int len);

int textwidth(const font_t *font, const char *str);
int textboxheight(const font_t *font, const char *str, int w);

vert2d_t* textverts(vert2d_t *v, const font_t *font, const char *str, int x, int y, uint32_t color);
vert2d_t* textbox(vert2d_t *v, const font_t *font, const char *str, int px, int y, int w, uint32_t color);*/

#endif
