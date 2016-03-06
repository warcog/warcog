#include "game.h"
#include "keycodes.h"
#include <stdio.h>

#define bg1 rgba(40, 53, 62, 255)
#define bg1_alpha rgba(80, 106, 124, 128)
//#define bg1_alpha2 rgba(120, 159, 186, 128)
#define bg2 rgba(30, 40, 47, 255)
#define bg2_alpha rgba(60, 80, 94, 128)
//#define bg2_alpha2 rgba(90, 120, 141, 192)

//#define bg3_alpha rgba(70, 93, 109, 128)
#define bg3_alpha rgba(80, 100, 110, 128)

#define white rgba(255, 255, 255, 255)
#define white_alpha rgba(255, 255, 255, 128)
#define lgray rgba(192, 192, 192, 255)
//#define lgray_alpha rgba(192, 192, 192, 128)
//#define gray_alpha rgba(128, 128, 128, 128)
#define black rgba(0, 0, 0, 255)
#define green rgba(0, 255, 0, 255)
#define green_dark rgba(0, 128, 0, 255)
#define red rgba(255, 0, 0, 255)
#define blue rgba(0, 0, 255, 255)

#define alpha rgba(0, 0, 0, 0)

#define cam_alpha rgba(128, 128, 128, 128)

enum {
    icon_button,
    icon_autocast,
    icon_passive,
    icon_disabled,
    icon_hold,
    icon_stop,
    icon_move,
};

typedef struct {
    uint8_t type, action;
    uint16_t icon;
} actionslot_t;

static const char* action_name[] = {"Move", "Stop", "Hold"};
static const char* action_desc[] = {"Move to a location", "Stop all current actions", "useless ability"};

static uint32_t div(uint32_t a, uint32_t b)
{
    //rounded division
    return (a + ((b + 1) / 2)) / b;
}

static int coord(float f)
{
    /* floor(f + 0.5) */
    int r;

    r = (f + 0.5);
    return r - (r > f);
}

static bool mouse_in(const game_t *g, int x, int y, int w, int h)
{
    x = g->mouse_pos.x - x;
    y = g->mouse_pos.y - y;
    return (g->mouse_in && x >= 0 && x < w && y >= 0 && y < h);
}

static vert2d_t solidw(int16_t x, int16_t y, uint16_t w, uint16_t h, rgba color)
{
    return quad(x, y, w, h, 0, color);
}

static vert2d_t solid(int16_t x, int16_t y, int16_t w, int16_t h, rgba color)
{
    return solidw(x, y, w - x, h - y, color);
}

static vert2d_t solidv(ivec2 x, ivec2 y, rgba color)
{
    return solid(x.x, y.x, x.y, y.y, color);
}

static vert2d_t icon(uint16_t x, uint16_t y, int16_t i)
{
    return quad(x, y, 64, 64, (2<<30)|(1<<24) | ((i >> 4) << 18) | ((i & 0xF) << 6), white);
}

static vert2d_t icon_push(uint16_t x, uint16_t y, int16_t i, int t, bool push)
{
    vert2d_t v = icon(x + (push ? t : 0), y + (push ? t : 0), i);
    v.tex |= (push << 26);
    return v;
}

static vert2d_t icon_sm(uint16_t x, uint16_t y, int16_t i)
{
    vert2d_t v = icon(x, y, i);
    v.tex |= (1 << 25);
    return v;
}

static vert2d_t* cooldown(vert2d_t *v, int x, int y, int w, double s)
{
    int tmp;

    if (s >= 0.5) {
        *v++ = quad(x, y, w/2, w, 0, cam_alpha);
        if (s >= 0.75) {
            *v++ = quad(x + w/2, y + w/2, w/2, w/2, 0, cam_alpha);
            if (s >= 0.875) {
                s = (s - 0.875) * 8.0;
                tmp = s * w / 2.0 + 0.5;

                *v++ = quad(x + w, y, -tmp, w/2, 0, cam_alpha);
                *v++ = quad(x + w - tmp, y + w/2, -(w/2 - tmp), -w/2, (1 << 27), cam_alpha);
            } else {
                *v++ = quad(x + w, y + w/2, -w/2, -(s - 0.75) * 8.0 * w/2, (1 << 27), cam_alpha);
            }
        } else {

        }
        return v;
    } else {
        return v;
    }

    /*if (s >= 0.25) {
        *v++ = quad(x, y, w, h, (1 << 27), cam_alpha)
    } else {
        return v;
    }*/

    //if (s < 0.125)
        //return *v++ = quad(x + w/2, y, -w/2, h, (1 << 27), cam_alpha), v;
}

/*static vert2d_t* textinput(vert2d_t *v, const game_t *g, const font_t *font, uint8_t id,
                           int x, int w, int y, int k, int line_height)
{
    const input_t *in = &g->input[id];
    uint32_t cx, tw;

    //bg = (mouse_in(g, x + k * 2, y, w - k * 4, line_height) && id != g->input_id) ? dark : black;
    *v++ = solidw(x + k * 2, y, w - k * 4, line_height, white);
    v = text_draw_centered(v, font, in->str, x, w, y + k, black);

    if (id == g->input_id) {
        tw = text_width2(font, in->str, &cx, g->input_at);
        *v++ = solidw(x + (w - tw) / 2 + cx, y + k, 1, font->y, black);
    }

    return v;
}

static bool texthit(game_t *g, const font_t *font, uint8_t id,
                    int x, int w, int y, int k, int line_height)
{
    const input_t *in = &g->input[id];
    if (mouse_in(g, x + k * 2, y, w - k * 4, line_height)) {
        g->input_id = id;

        x += (w - text_width(font, in->str)) / 2;
        g->input_at = text_fit(font, in->str, g->mouse_x - x);
        return 1;
    }
    return 0;
}*/

static actionslot_t slot(uint8_t type, uint8_t action, uint16_t icon)
{
    actionslot_t s = {type, action, icon};
    return s;
}

static void fill_slots(actionslot_t *s, const game_t *g, const entity_t *ent)
{
    const ability_t *a;

    int i, j;

    if (!ent) {
        for (i = 0; i < 12; i++)
            s[i] = slot(0, 0, icon_stop);

        return;
    }

    s[0] = slot(1, 0, icon_move);
    s[1] = slot(1, 1, icon_stop);
    s[2] = slot(1, 2, icon_hold);
    for (i = 3; i < 12; i++)
        s[i] = slot(0, 0, icon_stop);

    ability_loop(ent, a, i) {
        j = def(g, a)->slot;
        do { if (j >= 12) j = 0; if (!s[j].type) break; j++;} while (1); //TODO

        s[j] = slot(def(g, a)->target ? 3 : 2, (a - ent->ability), def(g, a)->icon);
    }
}

static void print_time(game_t *g, char *str, uint16_t time)
{
    sprintf(str, "%.2f", ((double) time - g->smooth) / (double) g->fps);
}

vert2d_t* gameui(game_t *g, vert2d_t *v)
{
    int i;
    const font_t *font, *font_big;
    const entity_t *ent;
    const ability_t *a;
    const state_t *s;
    actionslot_t slot[12];
    bool push;
    char str[32];

    struct {
        uint16_t icon;
        const char *name, *desc;
    } tooltip;

    int x, y;
    uint32_t w, h, k, t, icon_w;
    int sw, sh;

    if (!g->loaded)
        return v;

    if (g->select) {
        ivec2 s, t, x, y;

        s = g->sel_start;
        t = g->sel_end;

        x = (s.x < t.x) ? ivec2(s.x, t.x) : ivec2(t.x, s.x);
        y = (s.y < t.y) ? ivec2(s.y, t.y) : ivec2(t.y, s.y);

        *v++ = solid(x.x, y.x, x.y, y.y, white_alpha);
    }

    sw = g->width;
    sh = g->height;

    font = &g->font[font_ui];
    font_big = &g->font[font_ui_big];

    if (g->binding)
        v = text_draw_centered(v, font_big, "Enter a key", 0, sw, (sh - font_big->y) / 2, white);

    icon_w = div(sh * 64, 1080);

    /* health bars */
    for_entity(g, ent) {
        vec4 p;
        float x, y, w, h;
        ivec2 s, t;

        p = view_point(&g->view, add(ent->pos, vec3(0.0, 0.0, def(g, ent)->boundsheight)));
        x = (p.x * p.z + 1.0) * g->view.width / 2.0;
        y = (1.0 - p.y * p.w) * g->view.height / 2.0;
        w = p.w * g->view.height * 3.0;
        h = p.w * g->view.height / 2.0;

        s = ivec2(coord(x - w), coord(x + w));
        t = ivec2(coord(y - h), coord(y));

        if (s.x < 0 || s.y >= g->width || t.x < 0 || t.y >= g->height)
            continue;

        if (!ent->maxhp)
            continue;

        *v++ = solidv(s, t, green);
        s.x += (int) ent->hp * (s.y - s.x) / ent->maxhp;
        *v++ = solidv(s, t, red);
    }

    /* selected unit (health, mana, abilities, states, etc) */

    tooltip.name = 0;
    ent = g->nsel ? &g->entity[g->sel_ent[g->sel_first]] : 0;

    fill_slots(slot, g, ent);

    k = icon_w;
    for (i = 0; i < 12; i++) {
        x = sw - (4 - (i & 3)) * k;
        y = sh - (3 - (i >> 2)) * k;
        a = (slot[i].type & 2) ? &ent->ability[slot[i].action] : 0;

        push = g->slot_down == i;
        t = div(sh * 2, 1080);

        if (push)
            *v++ = solidw(x, y, k, k, black);
        *v++ = icon_push(x, y, slot[i].icon, t, push);
        *v++ = icon_push(x, y, (slot[i].type & 1) ? icon_button : icon_passive, t, push);
        if (a && a->cd) {
            print_time(g, str, a->cd);
            v = text_draw(v, font, str, x, y, white);
            //v = cooldown(v, x, y, k, ((double) a->cd - g->smooth) / a->cooldown);
        }

        if (slot[i].type && mouse_in(g, x, y, k, k)) {
            tooltip.icon = slot[i].icon;
            if (a) {
                tooltip.name = g->text + def(g, a)->tooltip.name;
                tooltip.desc = g->text + def(g, a)->tooltip.desc;
            } else {
                tooltip.name = action_name[slot[i].action];
                tooltip.desc = action_desc[slot[i].action];
            }
        }
    }

    if (ent) { /* health bar/states */
        w = div(sh, 3);
        h = font->y;
        x = (sw - w) / 2;
        y = sh;

        *v++ = icon(x, y - k, def(g, ent)->icon);
        if (mouse_in(g, x, y - k, k, k)) {
            tooltip.icon = def(g, ent)->icon;
            tooltip.name = g->text + def(g, ent)->tooltip.name;
            tooltip.desc = g->text + def(g, ent)->tooltip.desc;
        }

        x += k;
        w -= k;

        sprintf(str, "%u/%u", ent->mana, ent->maxmana);
        t = ent->maxmana ? div(w * ent->mana, ent->maxmana) : w;

        y -= h;
        *v++ = solidw(x, y, t, h, blue);
        *v++ = solidw(x + t, y, w - t, h, black);
        v = text_draw_centered(v, font, str, x, w, y, white);

        sprintf(str, "%u/%u", ent->hp, ent->maxhp);
        t = ent->maxhp ? div(w * ent->hp, ent->maxhp) : w;

        y -= h;
        *v++ = solidw(x, y, t, h, green_dark);
        *v++ = solidw(x + t, y, w - t, h, red);
        v = text_draw_centered(v, font, str, x, w, y, white);


        k = div(sh * 32, 1080);
        y -= k;

        state_loop(ent, s, i) {
            if (s->def < 0 || def(g, s)->icon < 0)
                continue;

            *v++ = icon_sm(x, y, def(g, s)->icon);
            *v++ = icon_sm(x, y, icon_passive);
            if (s->duration)
                v = cooldown(v, x, y, k, ((double) s->lifetime - g->smooth) / s->duration);

            if (mouse_in(g, x, y, k, k)) {
                tooltip.icon = def(g, s)->icon;
                tooltip.name = g->text + def(g, s)->tooltip.name;
                tooltip.desc = g->text + def(g, s)->tooltip.desc;
            }

            x += k;
        }
    }

    k = icon_w;

    x = sw - 5 * k;
    y = sh - k;
    for (i = 1; i < g->nsel; i++, x -= k) {
        ent = &g->entity[g->sel_ent[(g->sel_first + i) % g->nsel]];
        if (mouse_in(g, x, y, k, k)) {
            tooltip.icon = def(g, ent)->icon;
            tooltip.name = g->text + def(g, ent)->tooltip.name;
            tooltip.desc = g->text + def(g, ent)->tooltip.desc;
        }

        *v++ = icon(x, y, def(g, ent)->icon);

        t = ent->maxhp ? div(k * ent->hp, ent->maxhp) : k;
        *v++ = solidw(x, y + k - 4, t, 2, green);
        *v++ = solidw(x + t, y + k - 4, k - t, 2, red);

        t = ent->maxmana ? div(k * ent->mana, ent->maxmana) : k;
        *v++ = solidw(x, y + k - 2, t, 2, blue);
        *v++ = solidw(x + t, y + k - 2, k - t, 2, black);
    }

    if (tooltip.name) {
        w = k * 4;
        h = text_height(&g->font[font_ui], tooltip.desc, w) + k;
        x = sw - w;
        y = sh - h - 3 * k;

        *v++ = solidw(x, y, w, h, bg1_alpha);
        *v++ = icon(x, y, tooltip.icon);
        v = text_draw(v, &g->font[font_ui_big], tooltip.name, x + k, y, white);
        v = text_box(v, &g->font[font_ui], tooltip.desc, x, y + k, w, white);
    }

    /* minimap */
    w = div(sh * 256, 1080);

    *v++ = quad(0, sh - w, w, w, (3 << 30), white);

    {
        ivec2 a, b, c, d;
#define f(g, v, k) ({ \
    int x, y; \
    vec2 u; \
    u = view_intersect_ground(&g->view, v); \
    x = (int) (u.x * k) >> (g->map.size_shift + 4); \
    y = (int) (u.y * k) >> (g->map.size_shift + 4); \
    ivec2(x, y); \
})
        a = f(g, vec2(-1.0, -1.0), w);
        b = f(g, vec2(1.0, -1.0), w);
        c = f(g, vec2(-1.0, 1.0), w);
        d = f(g, vec2(1.0, 1.0), w);
#undef f

        *v++ = quad(a.x, sh - b.y, b.x - a.x, b.y - a.y, (1 << 27), cam_alpha);
        *v++ = quad(a.x, sh - b.y, c.x - a.x, b.y - a.y, 0, cam_alpha);
        *v++ = quad(c.x, sh - b.y, d.x - c.x, b.y - a.y, (1 << 27), cam_alpha);
    }

    return v;
}

static bool gameui_click(game_t *g, unsigned button)
{
    if (button == button_right)
        return 0;


    int i;
    const entity_t *ent;
    const font_t *font;
    //const ability_t *a;
    const state_t *s;
    actionslot_t slot[12];

    int x, y, w, h, k, icon_w;
    int sw, sh;
    ivec2 v;

    sw = g->width;
    sh = g->height;

    font = &g->font[font_ui];

    icon_w = div(sh * 64, 1080);

    ent = g->nsel ? &g->entity[g->sel_ent[g->sel_first]] : 0;

    fill_slots(slot, g, ent);

    k = icon_w;
    for (i = 0; i < 12; i++) {
        x = sw - (4 - (i & 3)) * k;
        y = sh - (3 - (i >> 2)) * k;
        //a = (slot[i].type & 2) ? &ent->ability[slot[i].action] : 0;

        if (mouse_in(g, x, y, k, k)) {
            if (slot[i].type & 1)
                g->slot_down = i;
            return 1;
        }
    }

    if (ent) { /* health bar/states */
        w = div(sh, 3);
        h = font->y;
        x = (sw - w) / 2;
        y = sh;

        if (mouse_in(g, x, y - k, k, k))
            return 1;

        x += k;
        w -= k;

        y -= h * 2;

        if (mouse_in(g, x, y, w, h * 2))
            return 1;

        k = div(sh * 32, 1080);
        y -= k;

        state_loop(ent, s, i) {
            if (s->def < 0 || def(g, s)->icon < 0)
                continue;

            if (mouse_in(g, x, y, k, k))
                return 1;

            x += k;
        }
    }

    k = icon_w;
    x = sw - 5 * k;
    y = sh - k;
    for (i = 1; i < g->nsel; i++, x -= k) {
        if (mouse_in(g, x, y, k, k)) {
            g->sel_first = (g->sel_first + i) % g->nsel;
            return 1;
        }
    }

    /* minimap */
    w = div(sh * 256, 1080);
    if (mouse_in(g, 0, sh - w, w, w)) {
        g->minimap_down = 1;


        v = g->mouse_pos;
        v = ivec2(v.x, sh - v.y);
        g->cam_pos = scale(vec2(v.x<<(g->map.size_shift + 4), v.y<<(g->map.size_shift + 4)), 1.0 / w);
        return 1;
    }

    return 0;
}

void gameui_move(game_t *g, int dx, int dy)
{
    int w, sh;
    ivec2 v;

    (void) dx;
    (void) dy;

    sh = g->height;

    if (g->minimap_down) {
        w = div(sh * 256, 1080);

        //TODO
        v = g->mouse_pos;
        v = clampv(ivec2(v.x, sh - v.y), ivec2(0, 0), ivec2(w, w));
        g->cam_pos = scale(vec2(v.x<<(g->map.size_shift + 4), v.y<<(g->map.size_shift + 4)), 1.0 / w);
    }
}

static void gameui_release(game_t *g, unsigned button)
{
    int i;
    const entity_t *ent;
    const ability_t *a;
    //const state_t *s;
    actionslot_t slot[12];

    int x, y, k, icon_w;
    int sw, sh;

    sw = g->width;
    sh = g->height;

    icon_w = div(sh * 64, 1080);

    ent = g->nsel ? &g->entity[g->sel_ent[g->sel_first]] : 0;

    fill_slots(slot, g, ent);

    k = icon_w;
    for (i = 0; i < 12; i++) {
        x = sw - (4 - (i & 3)) * k;
        y = sh - (3 - (i >> 2)) * k;
        //a = (slot[i].type & 2) ? &ent->ability[slot[i].action] : 0;

        if ((slot[i].type & 1) && mouse_in(g, x, y, k, k) && g->slot_down == i) {
            if (slot[i].type & 2) {
                a = &ent->ability[slot[i].action];
                if (button == button_middle)
                    g->binding = 1, g->bind_id = a->def;
                else
                    game_action(g, def(g, a)->target, (a - ent->ability) | 128,
                                def(g, a)->front_queue ? -1 : 0, (state & alt_mask) ? 1 : 0);
            } else {
                if (button == button_middle)
                    g->binding = -1, g->bind_id = slot[i].action;
                else
                    game_action(g, builtin_target[slot[i].action], slot[i].action, 0,
                                (state & alt_mask) ? 1 : 0);
            }
        }
    }

    g->slot_down = -1;
    g->minimap_down = 0;
}

void gameui_reset(game_t *g)
{
    g->slot_down = -1;
    g->minimap_down = 0;
}

bool game_ui_click(game_t *g, int button)
{
    if (g->loaded)
        return gameui_click(g, button);
    return 1;
}

bool ui_move(game_t *g, int dx, int dy)
{
    if (g->loaded) {
        gameui_move(g, dx, dy);
        return 0;
    }

    return 1;
}

void ui_buttonup(game_t *g, int button)
{
    gameui_release(g, button);
}

bool ui_wheel(game_t *g, double delta)
{
    (void) g;
    (void) delta;
    return 0;
}
