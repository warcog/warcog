#include "game.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "model.h"
#include "resource.h"

#include <util.h>

static const uint8_t def_sizes[num_defs] = {
    sizeof(entitydef_t),
    sizeof(abilitydef_t),
    sizeof(statedef_t),

    sizeof(effectdef_t),
    sizeof(particledef_t),
    sizeof(stripdef_t),
    sizeof(sounddef_t),
    sizeof(groundspritedef_t),
    sizeof(mdlmoddef_t),
    sizeof(attachmentdef_t),
};

static int16_t load_sound(game_t *g, data_t data, unsigned id, idmap_t *map)
{
    if (idmap_test(map, &id))
        return id;

    g->audio.sound[map->count] = read_sound(data, id);
    return idmap_add(map, id); /* add regardless of failure so we don't try again */
}

struct mdl_load_args {
    idmap_t *tmap;
    game_t *g;
    unsigned id;
};

static unsigned loadtex(void *user, unsigned tex)
{
    idmap_t *map;
    unsigned tid;

    map = ((struct mdl_load_args*) user)->tmap;

    tid = tex & 0x3FFF;
    if (idmap_test(map, &tid)) {
        tex = (tex & 0xC000) | tid;
        return tex;
    }

    tex = (tex & 0xC000) | idmap_add(map, tid);
    return tex;
}

static bool loadmdl(void *user, const void *mdl, size_t mdl_size, const uint16_t *index,
                    size_t nindex, const void *vert, size_t nvert)
{
    void *data;
    game_t *g;
    unsigned id;

    g = ((struct mdl_load_args*) user)->g;
    id = ((struct mdl_load_args*) user)->id;

    data = malloc(mdl_size);
    if (!data)
        return 0;
    memcpy(data, mdl, mdl_size);

    gfx_mdl_data(&g->gfx, id, index, vert, nindex, nvert);

    printf("model %u: nindex=%u, nvert=%u\n", id, (unsigned) nindex, (unsigned) nvert);
    g->model[id] = data;
    return 1;
}

static int16_t load_model(game_t *g, data_t data, unsigned id, idmap_t *map, idmap_t *tmap)
{
    data_t p;
    struct mdl_load_args arg;

    if (idmap_test(map, &id))
        return id;

    arg = (struct mdl_load_args){tmap, g, map->count};

    p = read_model(data, id);
    if (!model_load(p, loadtex, loadmdl, &arg))
        return -1;

    return idmap_add(map, id);
}

void loadmap(game_t *g)
{
    //TODO: multicall dec
    //TODO: as secondary thread
    uint8_t *data, *p, *p_prev;
    unsigned i, j, k;
    const mapdef_t *mapdef;
    texfile_t textures;
    data_t sounds, models;

    size_t size;
    int32_t len;

    entitydef_t *def;
    abilitydef_t *adef;
    statedef_t *sdef;

    idmap_t sndmap, mdlmap, texmap;
    uint16_t soundmap[max_sounds], modelmap[max_models], texturemap[max_textures]; //

    tatlas_t atlas;
    rgba *tex;
    int16_t idmap[max_textures];
    texinfo_t *info;

    //
    data = malloc(g->inflated);
    if (!data)
        goto fail;
    //
    len = decompress(data, g->data, g->inflated, g->size);
    if (len != (int32_t) g->inflated)
        goto fail_decompress;

    printf("decompressed %u->%u\n", g->size, g->inflated);

    //
#define advance(x) (p_prev = p, p += (x), (len -= (x)) < 0)
    p = data;

    /* defs */
    mapdef = (const mapdef_t*) p;
    if (advance(sizeof(mapdef_t)))
        goto fail_mapdef;

    size = 0;
    for (i = 0; i < num_defs; i++)
        size += mapdef->ndef[i] * def_sizes[i];

    if (advance(size))
        goto fail_defs;

    g->def[0] = malloc(size);
    if (!g->def[0])
        goto fail_defs;

    memcpy(g->def[0], p_prev, size);
    for (i = 0; i < num_defs - 1; i++)
        g->def[i + 1] = g->def[i] + mapdef->ndef[i] * def_sizes[i];

    /* */
    g->map_id = mapdef->map_id;
    bind_init(&g->bind, g->def[ability], mapdef->ndef[ability], mapdef->map_id);

    /* text */
    if (advance(mapdef->textlen))
        goto fail_text;

    g->text = malloc(mapdef->textlen);
    if (!g->text)
        goto fail_text;

    memcpy(g->text, p_prev, mapdef->textlen);

    /* map */
    size = (1 << (mapdef->size * 2)) * map_bytes_per_tile;
    if (advance(size))
        goto fail_map;

    if (!map_load(&g->map, p_prev, mapdef->size))
        goto fail_map;

    /* end of data */
    if (p != data + g->inflated)
        goto fail_data_end;
#undef advance

    /* resources */
    if (!read_texture_file(&textures, "textures"))
        goto fail_read_textures;

    if (!read_file(&sounds, "sounds", max_sounds * 4))
        goto fail_read_sounds;

    if (!read_file(&models, "models", max_models * 4))
        goto fail_read_models;

    tex = gfx_map(&g->gfx, tex_tiles);

    if (!load_tileset(&textures, tex, mapdef->tileset, countof(mapdef->tileset)))
        goto fail_load_tileset;

    gfx_unmap(&g->gfx, tex_tiles);

    //
    tex = gfx_map(&g->gfx, tex_icons);
    tatlas_init(&atlas, 1024, 1024, 64, idmap);

    for (i = 0; i < countof(mapdef->iconset) && mapdef->iconset[i] >= 0; i++)
        tatlas_add(&atlas, tex, &textures, mapdef->iconset[i]);

    //
    idmap_init(&sndmap, soundmap, countof(soundmap));
    idmap_init(&mdlmap, modelmap, countof(modelmap));
    idmap_init(&texmap, texturemap, countof(texturemap));

    for (i = 0; i < mapdef->ndef[entity]; i++) {
        def = idef(g, entity, i);
        if (def->model >= 0)
            def->model = load_model(g, models, def->model, &mdlmap, &texmap);

        if (def->icon >= 0)
            def->icon = tatlas_add(&atlas, tex, &textures, def->icon);

        for (j = 0; j < countof(def->voice); j++)
            for (k = 0; k < def->voice[j].count; k++)
                def->voice[j].sound[k] = load_sound(g, sounds, def->voice[j].sound[k], &sndmap);
    }

    for (i = 0; i < mapdef->ndef[ability]; i++) {
        adef = idef(g, ability, i);
        if (adef->icon >= 0)
            adef->icon = tatlas_add(&atlas, tex, &textures, adef->icon);
    }

    for (i = 0; i < mapdef->ndef[state]; i++) {
        sdef = idef(g, state, i);
        if (sdef->icon >= 0)
            sdef->icon = tatlas_add(&atlas, tex, &textures, sdef->icon);
    }

    gfx_unmap(&g->gfx, tex_icons);

    gfx_texture(&g->gfx, tex_mdl, 256, 256, texmap.count, filter_none, 0, format_rgba);
    tex = gfx_map(&g->gfx, tex_mdl);

    for (i = 0; i < texmap.count; i++) {
        info = get_texture_info(&textures, texmap.data[i]);
        if (!info || info->w != 256 || info->h != 256)
            continue; //

        if (!read_texture(&textures, info, &tex[256 * 256 * i]))
            continue; //
    }

    gfx_unmap(&g->gfx, tex_mdl);

    /* particle textures */
    //TODO
    particledef_t *pdef;
    uint32_t pinfo[256 * 4];

    tex = gfx_map(&g->gfx, tex_part);
    tatlas_init(&atlas, 1024, 1024, 64, idmap);

    for (i = 0; i < mapdef->ndef[effect + effect_particle]; i++) {
        pdef = edef(g, particle, i);
        pdef->texture = tatlas_add(&atlas, tex, &textures, pdef->texture);
        pinfo[i * 4 + 0] =
         (pdef->lifetime << 16) | (pdef->end_frame << 10) | (pdef->start_frame << 4) | pdef->texture;

        pinfo[i * 4 + 1] = pdef->color;
    }

    gfx_particleinfo(&g->gfx, pinfo, mapdef->ndef[effect + effect_particle]);

    gfx_unmap(&g->gfx, tex_part);

    for (i = 0; i < mapdef->ndef[effect + effect_sound]; i++)
        edef(g, sound, i)->sound = load_sound(g, sounds, edef(g, sound, i)->sound, &sndmap);

    gfx_mapverts(&g->gfx, g->map.vert, g->map.nvert);

    gfx_texture(&g->gfx, tex_map, g->map.size, g->map.size, 0, 0, 0, format_rg_int);
    gfx_copy(&g->gfx, tex_map, g->map.tile, g->map.size * g->map.size * 2);

    gfx_render_minimap(&g->gfx, (g->height * 256 + 540) / 1080);

    /* */
    free(models.data);
    free(sounds.data);
    close_texture_file(&textures);
    free(data);

    free(g->data); g->data = 0;
    g->loaded = 1;
    return;

fail_load_tileset:
    free(models.data);
fail_read_models:
    free(sounds.data);
fail_read_sounds:
    close_texture_file(&textures);
fail_read_textures:
fail_data_end:
    map_free(&g->map);
fail_map:
    free(g->text); g->text = 0;
fail_text:
    free(g->def[0]); g->def[0] = 0;
fail_defs:
fail_mapdef:
fail_decompress:
    free(data);
fail:
    printf("loadmap() failed\n");
}

static bool draw_tiles(void *user, vec3 min, vec3 max, treesearch_t *t)
{
    game_t *g = user;
    int res, size;

    res = view_intersect_box(&g->view, min, max, vec2(-1.0, 1.0), vec2(-1.0, 1.0));
    if (res < 0)
        return 0;

    size = (1 << (g->map.size_shift - t->depth));
    if (res || size == 1) {
        gfx_map_draw(&g->gfx, size * size * t->i, size * size);
        return 0;
    }
    return 1;
}

static void draw_entities(game_t *g)
{
    const entitydef_t *def;
    matrix4_t m;
    vec3 min, max;
    bool outline;

    gfx_mdl_begin(&g->gfx);

    //p = &players.player[self_id()];

    for_entity(g, ent) {
        def = def(g, ent);

        if (def->model < 0)
            continue;

        min = sub(ent->pos, vec3(def->boundsradius, def->boundsradius, 0.0));
        max = add(ent->pos, vec3(def->boundsradius, def->boundsradius, def->boundsheight));

        if (view_intersect_box(&g->view, min, max, vec2(-1.0, 1.0), vec2(-1.0, 1.0)) < 0)
            continue;

        outline = ((g->target & tf_entity) && g->target_ent == ent_id(g, ent));
        if (outline)
            gfx_mdl_outline(&g->gfx, vec3(1.0, 0.0, 0.0));

        m = view_object(&g->view, ent->pos, theta(ent->angle), def->modelscale);
        gfx_mdl(&g->gfx, def->model, &m.m[0][0]);

        model_draw(&g->gfx, g->model[def->model],
                   ent->anim & 127, ent->anim_len ? (double) ent->anim_frame / ent->anim_len : 0.0,
                   vec4(1.0, 0.0, 0.0, 1.0), ent->modcolor, 0xFFFF);

        if (outline)
            gfx_mdl_outline_off(&g->gfx);
    }
}

void game_frame(game_t *g)
{
    uint64_t t;
    double delta;
    vert2d_t *v, *vert;
    vec2 tmp;
    static double xd = 1.0;
    static int frames = 0;

    t = time_get();
    delta = time_seconds(t - g->time);
    g->time = t;

    xd -= delta;
    if (xd < 0.0) {
        xd += 1.0;
        printf("frames: %u\n", frames);
        frames = 0;
    }
    frames++;

    if (g->mouse_in) {
        tmp.x = (g->mouse_pos.x == g->width - 1) - (g->mouse_pos.x == 0);
        tmp.y = (g->mouse_pos.y == 0) - (g->mouse_pos.y == g->height - 1);
        add(&g->cam_pos, scale(tmp, delta * g->zoom * 2.0));
    }

    game_netframe(g);

    g->cam_pos = clamp2(g->cam_pos, 0.0, g->map.size * 16);

    view_params(&g->view, g->cam_pos, g->zoom, g->theta);
    audio_listener(&g->audio, g->view.ref);
    game_mouseover(g);

    vert = (vert2d_t*) g->tmp;
    v = gameui(g, vert);

    mapvert2_t mvert[4096];
    unsigned mcount = 0;

    if (g->loaded) {
        const entity_t *ent;
        mapvert2_t *v, *v_max;
        int i;
        double t;

        v = mvert;
        v_max = &mvert[4096];

        for (i = 0; i < g->nsel; i++) {
            ent = &g->entity[g->sel_ent[i]];
            v = map_circleverts(&g->map, v, v_max, ent->x, ent->y, def(g, ent)->boundsradius);
        }

        t = 8.0 - time_seconds(g->time - g->click_time) * 32.0;
        if (t > 0.0)
            v = map_circleverts(&g->map, v, v_max, g->click.x, g->click.y, t);

        mcount = v - mvert;
    }

    gfx_begin_draw(&g->gfx, g->width, g->height, &g->view.matrix.m[0][0],
                   mvert, mcount, &g->particle, vert, v - vert);

    if (g->loaded) {
        gfx_map_begin(&g->gfx);
        map_quad_iterate(&g->map, g, draw_tiles);

        draw_entities(g);
    }

    //gfx_depth_off(&g->gfx);

    //if (g->loaded)
    //    gfx_particles_draw(&g->gfx, , 1.0 / g->view.aspect, &g->view.matrix.m[0][0]);

    gfx_finish_draw(&g->gfx);
}

static void render_text(game_t *g, int h)
{
    void *data;

    data = gfx_map(&g->gfx, tex_text);
    text_rasterize(g->font, data, h);
    gfx_unmap(&g->gfx, tex_text);
}

bool game_init(game_t *g, unsigned w, unsigned h, unsigned argc, char *argv[])
{
    int i, j;
    entity_t *ent;
    addr_t addr;

    if (!time_init())
        return 0;

    if (!gfx_init(&g->gfx, w, h))
        return 0;

    gfx_texture(&g->gfx, tex_tiles, 1024, 1024, 0, filter_none, 0, format_rgba);
    gfx_texture(&g->gfx, tex_icons, 1024, 1024, 0, filter_none, 0, format_rgba);
    gfx_texture(&g->gfx, tex_part, 1024, 1024, 0, filter_none, 0, format_rgba);
    gfx_texture(&g->gfx, tex_text, 1024, 1024, 0, filter_none, 0, format_alpha);

    render_text(g, h);

    g->width = w;
    g->height = h;

    view_screen(&g->view, w, h);

    /* */
    g->zoom = 128.0;
    g->theta = -M_PI / 16.0;

    //TODO
    gameui_reset(g);

    /* */
    for (i = 0; i < 65535; i++) {
        ent = &g->entity[i];

        ent->def = -1;
        ent->voice_sound = -1;

        for (j = 0; j < 64; j++)
            ent->state[j].def = -1;

        for (j = 0; j < 16; j++)
            ent->ability[j].def = -1;
    }

    g->time = time_get();

    if (!game_netinit(g))
        goto cleanup;

    if (argc == 3) {
        if (net_getaddr(&addr, argv[1], argv[2]))
            game_directconnect(g, &addr);
        else
            printf("failed to resolve address\n");
    } else {
        printf("usage: %s [hostname] [port]\n", argv[0]);
         if (net_getaddr(&addr, "127.0.0.1", "1337"))
            game_directconnect(g, &addr);
    }

    return 1;

cleanup:
    return 0;
}

void game_resize(game_t *g, int w, int h)
{
    gfx_resize(&g->gfx, w, h);
    render_text(g, h);

    g->width = w;
    g->height = h;

    view_screen(&g->view, w, h);

    if (g->loaded)
        gfx_render_minimap(&g->gfx, (g->height * 256 + 540) / 1080);
}

void game_exit(game_t *g)
{
    gfx_done(&g->gfx);

    if (g->loaded)
        map_free(&g->map);

    game_disconnect(g);
}

#include "keycodes.h"

void game_keydown(game_t *g, uint32_t key)
{
    const entity_t *ent;
    const ability_t *a;
    unsigned i;
    bind_t b;

    if (key >= 255)
        return;

    if (g->nsel) {

    ent = &g->entity[g->sel_ent[g->sel_first]];

    /*for (i = 0; i < 3; i++) {
        if (key == g->builtin_key[i]) {
            game_action(g, builtin_target[i], i, 0, (g->key_state & alt_mask) ? 1 : 0);
            return;
        }
    }*/

    ability_loop(ent, a, i) {
        b = g->bind.data[a->def];
        if (bind_match(b, key, g->keys_down, g->keys_num)) {
            game_action(g, def(g, a)->target, (a - ent->ability) | 128,
                        def(g, a)->front_queue ? -1 : 0, (g->key_state & alt_mask) ? 1 : 0);
            break;
        }
    }
    }

    for (i = 0; i < g->keys_num; i++)
        if (key == g->keys_down[i])
            break;

    if (i == g->keys_num)
        g->keys_down[g->keys_num++] = key;
}

void game_keyup(game_t *g, uint32_t key)
{
    unsigned i, j, id;
    bind_t b;

    if (key >= 255)
        return;

    for (i = 0, j = 0; i < g->keys_num; i++)
        if (key != g->keys_down[i])
            g->keys_down[j++] = g->keys_down[i];
    assert(j + 1 == g->keys_num);
    g->keys_num = j;

    if (!g->binding)
        return;

    b.map = g->map_id;
    b.info = 0;
    if (g->binding < 0) {
        b.info |= bind_builtin;
        b.ability = g->bind_id;
        id = 0x8000 | g->bind_id;
    } else {
        b.ability = idef(g, ability, g->bind_id)->hash;
        id = g->bind_id;
    }

    j = g->keys_num > 2 ? 2 : g->keys_num;
    b.info |= j;
    for (i = 0; i < j; i++)
        b.keys[i] = g->keys_down[i];
    b.keys[2] = key;
    bind_set(&g->bind, id, b);

    printf("%u %u %u\n", id, b.info, b.keys[2]);

    g->binding = 0;
}

void game_keysym(game_t *g, uint32_t sym)
{
    /*input_t *in;
    char *p; */

    if (sym == key_f1)
        if (lockcursor(g, !g->mouse_locked))
            g->mouse_locked = !g->mouse_locked;

    /*if (g->input_id >= 0) {
        in = &g->input[g->input_id];
        p = in->str + g->input_at;

        switch (sym) {
        case key_left:
            if (g->input_at)
                g->input_at -= utf8_unlen(p);
            break;
        case key_right:
            if (g->input_at < in->len)
                g->input_at += utf8_len(p);
            break;
        }
        return;
    }*/

    if (sym == key_tab)
        g->sel_first = (g->sel_first + 1) % g->nsel;
}

void game_char(game_t *g, char *ch, int size)
{
    (void) g;
    (void) ch;
    (void) size;
    /*input_t *in;
    char *p;

    if (g->input_id < 0)
        return;

    in = &g->input[g->input_id];
    p = in->str + g->input_at;

    if (ch[0] == 8) { //backspace
        if (!g->input_at)
            return;

        size = utf8_unlen(p);
        memmove(p - size, p, in->len - g->input_at);
        in->len -= size;
        g->input_at -= size;
        in->str[in->len] = 0;
        return;
    }

    if (in->len + size >= in->max)
        return;

    memmove(p + size, p, in->len - g->input_at);
    memcpy(p, ch, size);

    in->len += size;
    g->input_at += size;
    in->str[in->len] = 0;

    printf("%.*s\n", size, ch); */
}

static void updateselection(game_t *g)
{
    const entitydef_t *def;
    vec3 min, max;
    vec2 sx, sy;
    ivec2 s, t;
    uint8_t j, k;
    uint16_t id, id_first;

    s = g->sel_start;
    t = g->sel_end;

    sx = add(scale((t.x >= s.x) ? vec2(s.x, t.x) : vec2(t.x, s.x), g->view.matrix2d[0]),
              vec2(-1.0, -1.0));
    sy = add(scale((t.y < s.y) ? vec2(s.y, t.y) : vec2(t.y, s.y), g->view.matrix2d[1]),
              vec2(1.0, 1.0));

    id_first = g->nsel ? g->sel_ent[g->sel_first] : 0xFFFF;
    k = 0xFF;

    j = 0;
    for_entity(g, ent) {
        def = def(g, ent);
        if (ent->owner != g->packet.id || !(def->uiflags & ui_control))
            continue;

        min = sub(ent->pos, vec3(def->boundsradius, def->boundsradius, 0.0));
        max = add(ent->pos, vec3(def->boundsradius, def->boundsradius, def->boundsheight));

        if (view_intersect_box(&g->view, min, max, sx, sy) < 0)
            continue;

        id = (ent - g->entity);

        if (id == id_first)
            k = j;

        g->sel_ent[j++] = id;
        if (j == 255)
            break;
    }

    if (j)
        g->nsel = j;

    if (k != 0xFF)
        g->sel_first = k;
}

void game_motion(game_t *g, int x, int y)
{
    ivec2 pos, d;

    pos = ivec2(x, y);
    d = sub(pos, g->mouse_pos);
    g->mouse_pos = pos;

    if (ui_move(g, d.x, d.y))
        return;

    if (g->select) {
        g->sel_end = clampv(pos, ivec2(0, 0), ivec2(g->width - 1, g->height - 1));
        updateselection(g);
    }

    if (g->middle)
        add(&g->cam_pos, scale(vec2(d.x, -d.y), -g->zoom * (0.375 / 128.0)));
}

static void playvoice(game_t *g, entity_t *ent, uint8_t i)
{
    const voice_t *v;

    if (time_seconds(g->time - g->voice_time) < 5.0)
        return;

    v = &def(g, ent)->voice[i];
    if (!v->count)
        return;

    if (ent->voice_sound >= 0)
        audio_stop(&g->audio, ent->voice_sound);

    ent->voice_sound = audio_play(&g->audio, v->sound[rand() % v->count], ent->pos, 0, 0);
    g->voice_time = g->time;
}

void game_button(game_t *g, int x, int y, int button, uint32_t state)
{
    uint8_t tf, target, priority;
    ability_t *a;
    int i;
    bool shift;
    entity_t *ent;

    g->key_state = state;
    shift = ((g->key_state & shift_mask) != 0);

    assert(x >= 0 && x < g->width);
    assert(y >= 0 && y < g->height);

    if (game_ui_click(g, button))
            return;

    if (button == button_left) {
        if (g->nsel) {
            ent = &g->entity[g->sel_ent[g->sel_first]];

            tf = (g->action_target & g->target);
            if (tf) {
                target = target_none;
                if (tf & tf_entity) {
                    target = target_ent;
                } else if (tf & tf_ground) {
                    target = target_pos;
                    g->click = g->target_pos;
                    g->click_time = g->time;
                }

                playvoice(g, ent, (g->action & 128) ? 2 : 1);
                game_netorder(g, g->action, target, shift ? 1 : g->action_queue, g->action_alt);
                g->action_target = 0;
                setcursor(g, 0);
                return;
            }

        }

        if (g->action_target)
            return;

        g->sel_start = g->sel_end = ivec2(x, y);
        g->select = 1;

        g->nsel = 0;
        if (g->target & tf_entity) {
            g->nsel = 1;
            g->sel_ent[0] = g->target_ent;
            g->sel_first = 0;

            playvoice(g, &g->entity[g->target_ent], 0);
        }
    }

    if (button == button_middle)
        g->middle = 1;

    if (button == button_right) {
        setcursor(g, 0);

        if (g->nsel) {
            ent = &g->entity[g->sel_ent[g->sel_first]];

            g->action_target = tf_ground | tf_entity;
            if (g->target_ent == ent_id(g, ent))
                g->action_target = tf_ground;
            g->action = 0;
            g->action_queue = 0;
            g->action_alt = 0;

            priority = 0;
            ability_loop(ent, a, i) {
                tf = g->target & def(g, a)->target;

                if (def(g, a)->priority <= priority)
                    continue;

                if ((tf & tf_entity) && g->target_ent == ent_id(g, ent))
                    tf &= ~tf_entity;

                if (tf) {
                    priority = def(g, a)->priority;
                    g->action_target = def(g, a)->target;
                    g->action = (a - ent->ability) | 128;
                    g->action_queue = def(g, a)->front_queue ? -1 : 0;
                }
            }

            tf = (g->action_target & g->target);
            if (tf) {
                target = target_none;
                if (tf & tf_entity) {
                    target = target_ent;
                } else if (tf & tf_ground) {
                    target = target_pos;
                    g->click = g->target_pos;
                    g->click_time = g->time;
                }

                playvoice(g, ent, (g->action & 128) ? 2 : 1);
                game_netorder(g, g->action, target, shift ? 1 : g->action_queue, g->action_alt);
                g->action_target = 0;
                setcursor(g, 0);
                return;
            }
        }

        g->action_target = 0;
    }
}

void game_buttonup(game_t *g, int button)
{
    ui_buttonup(g, button);

    if (button == button_left)
        g->select = 0;

    if (button == button_middle)
        g->middle = 0;
}

void game_wheel(game_t *g, double delta)
{
    if (ui_wheel(g, delta))
        return;

    g->zoom -= delta * 16.0;
    if (g->zoom < 16.0)
        g->zoom = 16.0;

    if (g->zoom > 512.0)
        g->zoom = 512.0;
}
