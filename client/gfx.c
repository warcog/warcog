#include "gfx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef USE_VULKAN

bool create_window(int width, int height);
void destroy_window(void);
void swap_buffers(void);

#ifdef WIN32
//#define WINGDIAPI
//#define APIENTRY
#include <GL/gl.h>
#include <GL/glext.h>
#include "win32/wgl.h"
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#endif

static void debug_infolog(GLuint shader, const char *data)
{
    #ifndef NDEBUG
    char infolog[256];

    glGetShaderInfoLog(shader, sizeof(infolog), NULL, (void*) infolog);

    printf("shader loading failed:\n%s\n", data);
    printf("Infolog: %s\n", infolog);
    #else
    (void) shader;
    (void) data;
    #endif
}

static bool compile_shader(GLuint shader, const char *data)
{
    GLint status;

    glShaderSource(shader, 1, &data, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        debug_infolog(shader, data);
        return 0;
    }

    return 1;
}

static bool shader_load(shader_t *res, const char *dvert, const char *dgeom, const char *dfrag)
{
    GLuint prog, vert, geom, frag;
    GLint status;

    vert = glCreateShader(GL_VERTEX_SHADER);
    if (!vert)
        return 0;

    if (!compile_shader(vert, dvert))
        goto EXIT_DELETE_VERT;

    frag = glCreateShader(GL_FRAGMENT_SHADER);
    if (!frag)
        goto EXIT_DELETE_VERT;

    if (!compile_shader(frag, dfrag))
        goto EXIT_DELETE_FRAG;

    geom = 0;
    if (dgeom) {
        geom = glCreateShader(GL_GEOMETRY_SHADER);
        if (!geom)
            goto EXIT_DELETE_FRAG;

        if (!compile_shader(geom, dgeom))
            goto EXIT_DELETE_GEOM;
    }

    prog = glCreateProgram();
    if (!prog)
        goto EXIT_DELETE_GEOM;


    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    if (geom)
        glAttachShader(prog, geom);

    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (!status) {
        debug_infolog(prog, "glLinkProgram");
        glDeleteProgram(prog);
        goto EXIT_DELETE_GEOM;
    }

    res->prog = prog;
    res->matrix = glGetUniformLocation(prog, "matrix");
    res->var0 = glGetUniformLocation(prog, "var0");
    res->var1 = glGetUniformLocation(prog, "var1");

    if (geom) {
        glDetachShader(prog, geom);
        glDeleteShader(geom);
    }

    glDetachShader(prog, frag);
    glDeleteShader(frag);
    glDetachShader(prog, vert);
    glDeleteShader(vert);
    return 1;

EXIT_DELETE_GEOM:
    if (geom)
        glDeleteShader(geom);
EXIT_DELETE_FRAG:
    glDeleteShader(frag);
EXIT_DELETE_VERT:
    glDeleteShader(vert);
    return 0;
}

bool gfx_init(gfx_t *g, unsigned w, unsigned h)
{
    unsigned i;
    const shader_t *s;
    data_t file;
    uint16_t *lengths;
    void *data;
    const char *vert, *geom, *frag;

    if (!create_window(w, h))
        return 0;

    glDepthFunc(GL_LEQUAL);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    if (!read_file(&file, "shaders-gl", 0))
        assert(0);

    lengths = data = file.data;

    if (!dp_read(&file, shader_objcount * 2))
        assert(0);

    for (i = 0; i < num_shader; i++) {
        vert = dp_read(&file, *lengths++);
        assert(vert);

        frag = dp_read(&file, *lengths++);
        assert(frag);

        geom = 0;
        if (shader_hasgeom & (1 << i)) {
            geom = dp_read(&file, *lengths++);
            assert(geom);
        }

        if (!shader_load(&g->shader[i], vert, geom, frag))
            goto fail; //TODO
    }

    free(data);

    s = &g->shader[so_post];
    glUseProgram(s->prog);
    glUniform1i(s->var1, 1);

    s = &g->shader[so_2d];
    glUseProgram(s->prog);
    glUniform1i(s->var1, 1);
    glUniform1i(glGetUniformLocation(s->prog, "var2"), 2);

    s = &g->shader[so_minimap];
    glUseProgram(s->prog);
    glUniform1i(s->var1, 1);

    s = &g->shader[so_model];
    g->uniform_outline = glGetUniformLocation(s->prog, "outline");
    g->uniform_layer = glGetUniformLocation(s->prog, "layer");
    g->uniform_rot = glGetUniformLocation(s->prog, "r");
    g->uniform_trans = glGetUniformLocation(s->prog, "d");

    glGenBuffers(countof(g->vbo), g->vbo);
    glGenVertexArrays(countof(g->vao), g->vao);
    glGenTextures(countof(g->tex), g->tex);
    glGenFramebuffers(countof(g->fbo), g->fbo);
    glGenRenderbuffers(1, &g->depth);

    /* 2d */
    glBindBuffer(GL_ARRAY_BUFFER, g->vbo[vo_2d]);
    glBufferData(GL_ARRAY_BUFFER, 0x40000, 0, GL_STREAM_DRAW); //TODO 0x40000
    glBindVertexArray(g->vao[vo_2d]);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 4, GL_SHORT, GL_FALSE, 16, 0);
    glVertexAttribIPointer(1, 2, GL_UNSIGNED_INT, 16, (void*) 8);

    /* part */
    glBindBuffer(GL_ARRAY_BUFFER, g->vbo[vo_part]);
    glBufferData(GL_ARRAY_BUFFER, 0x10000, 0, GL_STREAM_DRAW); //TODO 0x10000
    glBindVertexArray(g->vao[vo_part]);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 16, 0);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, 16, (void*) 12);

    /*...  */
    glBindBuffer(GL_ARRAY_BUFFER, g->vbo[vo_sel]);
    glBufferData(GL_ARRAY_BUFFER, 0x40000, 0, GL_STREAM_DRAW); //TODO 0x40000
    glBindVertexArray(g->vao[vo_sel]);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 16, 0);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_FALSE, 16, (void*) 4);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 16, (void*) 8);

    /* map */
    glBindBuffer(GL_ARRAY_BUFFER, g->vbo[vo_map]);
    //
    glBindVertexArray(g->vao[vo_map]);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 8, 0);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_FALSE, 8, (void*) 4);

    /* */
    for (i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, g->tex[i ? tex_screen1 : tex_screen0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    }

    glBindRenderbuffer(GL_RENDERBUFFER, g->depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, w, h);

    glBindFramebuffer(GL_FRAMEBUFFER, g->fbo[0]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g->tex[tex_screen0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, g->tex[tex_screen1], 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, g->depth);

    return glGetError() == 0;

fail:
    //TODO
    return 0;
}

bool gfx_resize(gfx_t *g, unsigned w, unsigned h)
{
    glBindTexture(GL_TEXTURE_2D, g->tex[tex_screen0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    glBindTexture(GL_TEXTURE_2D, g->tex[tex_screen1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, g->depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, w, h);
    return 1;
}

void gfx_done(gfx_t *g)
{
    unsigned i;

    for (i = 0; i < num_shader; i++)
        glDeleteProgram(g->shader[i].prog);

    glDeleteBuffers(countof(g->vbo), g->vbo);
    glDeleteVertexArrays(countof(g->vao), g->vao);
    glDeleteTextures(countof(g->tex), g->tex);
    glDeleteFramebuffers(countof(g->fbo), g->fbo);
    glDeleteRenderbuffers(1, &g->depth);

    destroy_window();
}

void gfx_render_minimap(gfx_t *g, unsigned w)
{
    const shader_t *s;
    float scale;

    glBindTexture(GL_TEXTURE_2D, g->tex[tex_minimap]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, w, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, g->fbo[1]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,g->tex[tex_minimap],0);

    s = &g->shader[so_minimap];
    scale = 1.0 / w;

    glBindTexture(GL_TEXTURE_2D, g->tex[tex_map]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g->tex[tex_tiles]);
    glActiveTexture(GL_TEXTURE0);

    glDisable(GL_BLEND);

    glUseProgram(s->prog);
    glUniform1fv(s->var0, 1, &scale);

    glViewport(0, 0, w, w);

    glBindFramebuffer(GL_FRAMEBUFFER, g->fbo[1]);
    glBindVertexArray(g->vao[num_vao]);
    glDrawArrays(GL_POINTS, 0, 1);

    glEnable(GL_BLEND);
}

void gfx_mdl_data(gfx_t *g, unsigned id, const uint16_t *index, const void *vert,
                 unsigned nindex, unsigned nvert)
{
    glBindVertexArray(g->vao[vo_mdl + id]);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g->vbo[vo_mdl_elements +  id]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, nindex * 2, index, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, g->vbo[vo_mdl + id]);
    glBufferData(GL_ARRAY_BUFFER, nvert * vert_size, vert, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vert_size, 0);
    glVertexAttribPointer(1, 2, GL_UNSIGNED_SHORT, GL_FALSE, vert_size, (void*) 12);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_FALSE, vert_size, (void*) 16);
    glVertexAttribIPointer(3, 4, GL_UNSIGNED_BYTE, vert_size, (void*) 20);
}

void gfx_particleinfo(gfx_t *g, GLuint *info, uint32_t count)
{
    const shader_t *s;

    s = &g->shader[so_particle];
    glUseProgram(s->prog);
    glUniform4uiv(s->var1, count, info);
}

void gfx_mapverts(gfx_t *g, mapvert_t *vert, uint32_t nvert)
{
    glBindBuffer(GL_ARRAY_BUFFER, g->vbo[vo_map]);
    glBufferData(GL_ARRAY_BUFFER, nvert * sizeof(mapvert_t), vert, GL_STATIC_DRAW);
}

void gfx_begin_draw(gfx_t *g, uint32_t w, uint32_t h, float *matrix,
                    mapvert2_t *mv, unsigned mcount,
                    psystem_t *p, vert2d_t *v, unsigned count)
{
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearDepth(1.0);

    glViewport(0, 0, w, h);

    glBindFramebuffer(GL_FRAMEBUFFER, g->fbo[0]);
    glDrawBuffer(GL_COLOR_ATTACHMENT1);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    g->w = w;
    g->h = h;
    g->vert2d_count = count;
    g->particle_count = p->count;
    g->sel_count = mcount;
    memcpy(g->matrix, matrix, 64);

    glBindBuffer(GL_ARRAY_BUFFER, g->vbo[vo_2d]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(vert2d_t), v);

    glBindBuffer(GL_ARRAY_BUFFER, g->vbo[vo_part]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, p->count * sizeof(*p->data), p->data);

    glBindBuffer(GL_ARRAY_BUFFER, g->vbo[vo_sel]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, mcount * sizeof(*v), mv);
}

void gfx_finish_draw(gfx_t *g)
{
    const shader_t *s;
    vec2 mat;
    float sc;

    s = &g->shader[so_mapx];
    glUseProgram(s->prog);
    glUniformMatrix4fv(s->matrix, 1, GL_FALSE, g->matrix);
    glBindVertexArray(g->vao[vo_sel]);
    glDrawArrays(GL_POINTS, 0, g->sel_count);

    glDisable(GL_DEPTH_TEST);

    sc = (float) g->h / g->w;

    s = &g->shader[so_particle];
    glUseProgram(s->prog);
    glUniformMatrix4fv(s->matrix, 1, GL_FALSE, g->matrix);
    glUniform1fv(s->var0, 1, &sc);
    glBindVertexArray(g->vao[vo_part]);
    glBindTexture(GL_TEXTURE_2D, g->tex[tex_part]);
    glDrawArrays(GL_POINTS, 0, g->particle_count);

    glDisable(GL_BLEND);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, g->tex[tex_screen0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g->tex[tex_screen1]);
    glActiveTexture(GL_TEXTURE0);

    s = &g->shader[so_post];

    glUseProgram(s->prog);

    mat = vec2(1.0 / g->w, 1.0 / g->h);
    glUniform2fv(s->matrix, 1, &mat.x);
    glBindVertexArray(g->vao[num_vao]);
    glDrawArrays(GL_POINTS, 0, 1);

    /* */
    glBindTexture(GL_TEXTURE_2D, g->tex[tex_text]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, g->tex[tex_minimap]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g->tex[tex_icons]);
    glActiveTexture(GL_TEXTURE0);

    glEnable(GL_BLEND);

    /* */
    s = &g->shader[so_2d];
    glUseProgram(s->prog);

    mat = vec2(2.0 / g->w, -2.0 / g->h);
    glUniform2fv(s->matrix, 1, &mat.x);

    sc = g->h / 1080.0;
    glUniform1fv(s->var0, 1, &sc);

    glBindVertexArray(g->vao[vo_2d]);
    glDrawArrays(GL_POINTS, 0, g->vert2d_count);

   if (glGetError() != 0)
        printf("glGetError != 0\n");

    swap_buffers();
}

void gfx_map_begin(gfx_t *g)
{
    const shader_t *s;

    s = &g->shader[so_map];
    glUseProgram(s->prog);
    glUniformMatrix4fv(s->matrix, 1, GL_FALSE, g->matrix);
    glBindVertexArray(g->vao[vo_map]);
    glBindTexture(GL_TEXTURE_2D, g->tex[tex_tiles]);
}

void gfx_map_draw(gfx_t *g, uint32_t offset, uint32_t count)
{
    (void) g;
    glDrawArrays(GL_POINTS, offset, count);
}

void gfx_mdl_begin(gfx_t *g)
{
    glUseProgram(g->shader[so_model].prog);
    glBindTexture(GL_TEXTURE_2D_ARRAY, g->tex[tex_mdl]);
}

void gfx_mdl(gfx_t *g, unsigned id, float *matrix)
{
    glBindVertexArray(g->vao[vo_mdl + id]);
    glUniformMatrix4fv(g->shader[so_model].matrix, 1, GL_FALSE, matrix);
}

void gfx_mdl_outline(gfx_t *g, vec3 color)
{
    const GLenum bufs[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};

    glDrawBuffers(2, bufs);
    glUniform3fv(g->uniform_outline, 1, &color.x);
}

void gfx_mdl_outline_off(gfx_t *g)
{
    (void) g;
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
}

void gfx_mdl_params(gfx_t *g, vec4 *rot, vec4 *tr_sc, uint8_t count, vec4 color)
{
    glUniform4fv(g->uniform_rot, count, rot->v);
    glUniform4fv(g->uniform_trans, count, tr_sc->v);
    glUniform4fv(g->shader[so_model].var0, 1, &color.x);
}

void gfx_mdl_texture(gfx_t *g, uint16_t tex, vec4 teamcolor)
{
    const float v[5] = {0.0, 0.0, 0.0, 0.0, 1.0};
    float layer;

    glUniform4fv(g->shader[so_model].var1, 1,
                (tex & 0x8000) ? &teamcolor.x : ((tex & 0x4000) ? v : v + 1));

    layer = tex & 0x3FFF;
    glUniform1fv(g->uniform_layer, 1, &layer);
}

void gfx_mdl_draw(gfx_t *g, uint32_t start, uint32_t end)
{
    (void) g;
    glDrawElements(GL_TRIANGLES, end - start, GL_UNSIGNED_SHORT, (void*)(size_t)(start * 2));
}

void gfx_texture(gfx_t *g, unsigned id, unsigned width, unsigned height, unsigned layers,
                 unsigned filter, unsigned mipmap, unsigned format)
{
    static const GLint swizzle[] = {GL_ONE, GL_ONE, GL_ONE, GL_RED};
    unsigned target, fmt, internal;
    texture_t *tex;

    tex = &g->texinfo[id];

    target = layers ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
    fmt = (format == format_rg_int) ? GL_RG_INTEGER :
          (format == format_alpha) ? GL_RED :
          GL_RGBA;
    internal = (format == format_rg_int) ? GL_RG8UI : fmt;

    glBindTexture(target, g->tex[id]);

    if (format == format_alpha)
        glTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
    if (mipmap) {
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, mipmap);
    } else {
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR : GL_NEAREST);
    }
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    if (layers)
        glTexImage3D(target, 0, internal, width, height, layers, 0, fmt, GL_UNSIGNED_BYTE, 0);
    else
        glTexImage2D(target, 0, internal, width, height, 0, fmt, GL_UNSIGNED_BYTE, 0);
    if (mipmap)
        glGenerateMipmap(target);

    tex->width = width;
    tex->height = height;
    tex->layers = layers;
    tex->format = format;
}

void* gfx_map(gfx_t *g, unsigned id)
{
    texture_t *tex;
    unsigned size_per_pixel, layers;
    void *data;

    tex = &g->texinfo[id];

    size_per_pixel = (tex->format == format_rg_int) ? 2 :
                     (tex->format == format_alpha) ? 1 :
                     4;
    layers = tex->layers ? tex->layers : 1;

    data = realloc(g->map, tex->width * tex->height * layers * size_per_pixel);
    if (data)
        g->map = data;

    return data;
}

void gfx_unmap(gfx_t *g, unsigned id)
{
    gfx_copy(g, id, g->map, 0);
}

void gfx_copy(gfx_t *g, unsigned id, void *data, unsigned size)
{
    (void) size;

    texture_t *tex;
    unsigned fmt, target;

    tex = &g->texinfo[id];

    target = tex->layers ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
    fmt = (tex->format == format_rg_int) ? GL_RG_INTEGER :
          (tex->format == format_alpha) ? GL_RED :
          GL_RGBA;

    glBindTexture(target, g->tex[id]);
    if (tex->layers)
        glTexSubImage3D(target, 0, 0, 0, 0, tex->width, tex->height, tex->layers,
                        fmt, GL_UNSIGNED_BYTE, g->map);
    else
        glTexSubImage2D(target, 0, 0, 0, tex->width, tex->height, fmt, GL_UNSIGNED_BYTE, data);
}

#else

#define rgba_format VK_FORMAT_R8G8B8A8_UNORM
#define alpha_format VK_FORMAT_R8_UNORM
#define depth_format VK_FORMAT_D16_UNORM

struct uniform_data {
    matrix4_t MVP;
    vec2 matrix;
    float var0, texture;
    vec4 team_color;
    vec4 color;

    matrix4_t m0;
    vec4 r0[126];
    vec4 d0[126];

    uint32_t info[256 * 4];
};
#define uniform_offset(x) offsetof(struct uniform_data, x)
#define uniform_size sizeof(struct uniform_data)

VkResult create_window(int width, int height, VkInstance inst, VkSurfaceKHR *surface);
void destroy_window(VkInstance inst, VkSurfaceKHR surface);

static void destroy_swapchain(gfx_t *g)
{
    vk_destroy(g->device, g->sc_view[0]);
    vk_destroy(g->device, g->sc_view[1]);
    vk_destroy(g->device, g->swapchain);
}

static VkResult set_swapchain(gfx_t *g, unsigned w, unsigned h)
{
    unsigned count;
    VkResult err;
    VkSwapchainKHR swapchain;
    VkImage image[2];
    VkImageView view[2];

    err = vk_create_swapchain(g->device, &swapchain,
        .surface = g->surface,
        .minImageCount = 2, //TODO apply min/max values
        .imageFormat = g->fmt.format,
        .imageColorSpace = g->fmt.colorSpace,
        .imageExtent = {.width = w, .height = h},
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .imageArrayLayers = 1,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR, //TODO choose
        .oldSwapchain = 0, //g->swapchain TODO destroying old swapchain=buggy?
        .clipped = 1,
    );
    if (err)
        return err;

    //TODO verify that image count is 2..

    count = 2;
    err = vk_get_images(g->device, swapchain, &count, image);
    if (err)
        goto fail_destroy_sc;

    err = vk_create_view(g->device, &view[0], .format = g->fmt.format, .image = image[0]);
    if (err)
        goto fail_destroy_sc;

    err = vk_create_view(g->device, &view[1], .format = g->fmt.format, .image = image[1]);
    if (err)
        goto fail_destroy_view;

    //if (g->swapchain)
    //    destroy_swapchain(g);

    g->swapchain = swapchain;
    g->sc_image[0] = image[0];
    g->sc_image[1] = image[1];
    g->sc_view[0] = view[0];
    g->sc_view[1] = view[1];
    return 0;

fail_destroy_view:
    vk_destroy(g->device, view[0]);
fail_destroy_sc:
    vk_destroy(g->device, swapchain);
    return err;
}

static VkResult allocate(gfx_t *g, VkDeviceMemory *res, VkMemoryRequirements *reqs, bool visible)
{
    unsigned bits, i;

    for (i = 0, bits = reqs->memoryTypeBits; i < 32; i++, bits >>= 1) {
        if (!(bits & 1))
            continue;

        if (visible &&
        !(g->mem_props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
            continue;

        return vk_alloc(g->device, res, .allocationSize = reqs->size, .memoryTypeIndex = i);
    }

    return VK_ERROR_FORMAT_NOT_SUPPORTED;
}

static VkResult set_texture(gfx_t *g, texture_t *tex, unsigned width, unsigned height,
                            unsigned layers, unsigned format)
{
    VkImage image;
    VkDeviceMemory mem;
    VkImageView view;

    VkImageUsageFlags usage;
    VkFormat fmt;
    VkImageTiling tiling;
    VkImageAspectFlags flags;
    VkMemoryRequirements mem_reqs;
    VkResult err;

    usage = (format == format_depth_render) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT :
            (format == format_rgba_render) ? (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT) :
            VK_IMAGE_USAGE_SAMPLED_BIT;

    fmt = (format == format_depth_render) ? depth_format :
          (format == format_alpha) ? VK_FORMAT_R8_UNORM :
          (format == format_rg_int) ? VK_FORMAT_R8G8_UINT :
          (format == format_rgba_render) ? g->fmt.format :
          VK_FORMAT_R8G8B8A8_UNORM;

    flags = (format == format_depth_render) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    tiling = (format >= format_rgba_render) ? VK_IMAGE_TILING_OPTIMAL : VK_IMAGE_TILING_LINEAR;

    err = vk_create_image(g->device, &image, .imageType = VK_IMAGE_TYPE_2D, .format = fmt,
                         .extent = {width, height, 1}, .mipLevels = 1, .arrayLayers = layers,
                         .samples = VK_SAMPLE_COUNT_1_BIT, .tiling = tiling, .usage = usage);
    if (err)
        return err;

    vk_mem_reqs(g->device, image, &mem_reqs);

    err = allocate(g, &mem, &mem_reqs, (format < format_rgba_render));
    if (err)
        goto fail_image;

    err = vk_bind(g->device, image, mem);
    if (err)
        goto fail_mem;

    //TODO swizzling instead of shader op
    err = vk_create_view(g->device, &view, .image = image, .format = fmt,
                         .subresourceRange.aspectMask = flags);
    if (err)
        goto fail_mem;

    if (tex->image) {
        vk_destroy(g->device, tex->view); //not needed
        vk_destroy(g->device, tex->image);
        vk_destroy(g->device, tex->mem);
    }

    tex->image = image;
    tex->mem = mem;
    tex->view = view;
    return 0;

fail_mem:
    vk_destroy(g->device, mem);
fail_image:
    vk_destroy(g->device, image);
    return err;
}

static VkResult render_pass(VkDevice dev, VkRenderPass *res, VkFormat format, VkFormat depth)
{
    static const VkAttachmentReference color_reference = {
        .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    static const VkAttachmentReference depth_reference = {
        .attachment = 1, .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentDescription attachments[2];
    VkSubpassDescription subpass;

    attachments[0] = vk_attachment(.format = format,
                    .loadOp = depth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    if (depth) {
        attachments[1] = vk_attachment(.format = depth,
                       .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                       .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                       .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }
    subpass = vk_subpass(1, &color_reference, depth ? &depth_reference : 0);

    return vk_create_renderpass(dev, res,
                                .attachmentCount = depth ? 2 : 1,
                                .pAttachments = attachments,
                                .subpassCount = 1,
                                .pSubpasses = &subpass);
}

static VkResult create_buffer(gfx_t *g, buffer_t *res, VkDeviceSize size, VkBufferUsageFlags usage)
{
    VkResult err;
    VkBuffer buf;
    VkMemoryRequirements mem_reqs;
    VkDeviceMemory mem;

    err = vk_create_buffer(g->device, &buf, .size = size, .usage = usage);
    if (err)
        return err;

    vk_mem_reqs(g->device, buf, &mem_reqs);

    err = allocate(g, &mem, &mem_reqs, 1);
    if (err)
        goto fail_buf;

    err = vk_bind(g->device, buf, mem);
    if (err)
        goto fail_mem;

    res->buf = buf;
    res->mem = mem;
    return 0;

fail_mem:
    vk_destroy(g->device, mem);
fail_buf:
    vk_destroy(g->device, buf);
    return err;
}

static void pipelines(gfx_t *g)
{
    //TODO cant render without scissor??
    static const VkDynamicState states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    static const VkPipelineDynamicStateCreateInfo dyns = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pDynamicStates = states,
        .dynamicStateCount = countof(states),
    };

    static const VkVertexInputBindingDescription vi_bindings[] = {
        {.binding = 0, .stride = 24, .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
    };

    static const VkVertexInputAttributeDescription vi_attrs[] = {
        {.binding = 0, .location = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0},
        {.binding = 0, .location = 1, .format = VK_FORMAT_R16G16_USCALED, .offset = 12},
        {.binding = 0, .location = 2, .format = VK_FORMAT_R8G8B8A8_USCALED, .offset = 16},
        {.binding = 0, .location = 3, .format = VK_FORMAT_R8G8B8A8_UINT, .offset = 20},
    };

    static const VkVertexInputBindingDescription vi_bind_8 = {
        .binding = 0, .stride = 8, .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    static const VkVertexInputBindingDescription vi_bind_16 = {
        .binding = 0, .stride = 16, .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    static const VkVertexInputAttributeDescription vi_attr_2d[] = {
        {.binding = 0, .location = 0, .format = VK_FORMAT_R16G16B16A16_SSCALED, .offset = 0},
        {.binding = 0, .location = 1, .format = VK_FORMAT_R32G32_UINT, .offset = 8},
    };

    static const VkVertexInputAttributeDescription vi_attr_part[] = {
        {.binding = 0, .location = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0},
        {.binding = 0, .location = 1, .format = VK_FORMAT_R32_UINT, .offset = 12},
    };

    static const VkVertexInputAttributeDescription vi_attr_map[] = {
        {.binding = 0, .location = 0, .format = VK_FORMAT_R32_UINT, .offset = 0},
        {.binding = 0, .location = 1, .format = VK_FORMAT_R8G8B8A8_USCALED, .offset = 4},
    };

    static const VkVertexInputAttributeDescription vi_attr_mapx[] = {
        {.binding = 0, .location = 0, .format = VK_FORMAT_R32_UINT, .offset = 0},
        {.binding = 0, .location = 1, .format = VK_FORMAT_R8G8B8A8_USCALED, .offset = 4},
        {.binding = 0, .location = 2, .format = VK_FORMAT_R32G32_SFLOAT, .offset = 8},
    };

    static const VkPipelineRasterizationStateCreateInfo rs = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,//VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .depthBiasEnable = VK_FALSE,
    };

    static const VkPipelineColorBlendAttachmentState att_state[] = {
        {
        .colorWriteMask = 0xf,
        .blendEnable = 1,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        //.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        //.alphaBlendOp = VK_BLEND_OP_ADD,
        }
    };
    static const VkPipelineColorBlendStateCreateInfo cb = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = att_state,
    };

    static const VkPipelineViewportStateCreateInfo vp = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    static const VkPipelineMultisampleStateCreateInfo ms = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    static const VkDescriptorPoolSize counts[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8},
    };

    VkResult err;
    VkPipelineDepthStencilStateCreateInfo ds;
    VkPipelineInputAssemblyStateCreateInfo ia;
    VkPipelineVertexInputStateCreateInfo vi;
    VkPipelineShaderStageCreateInfo stage[3];
    VkGraphicsPipelineCreateInfo pipeline;
    data_t file;
    void *data, *p;
    uint16_t *lengths;
    unsigned i, j, len;

    VkDescriptorSetLayoutBinding layout[9];
    VkDescriptorBufferInfo uniform;
    VkWriteDescriptorSet write;

    layout[0] = vk_layout_uniform(0, 1, VK_SHADER_STAGE_ALL_GRAPHICS);
    for (i = 1; i < countof(layout); i++)
        layout[i] = vk_layout_sampler(i, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    err = vk_create_desc_layout(g->device, &g->desc_layout,
                                .bindingCount = countof(layout), .pBindings = layout);
    assert(!err);

    err = vk_create_pipe_layout(g->device, &g->pipeline_layout,
                                .setLayoutCount = 1, .pSetLayouts = &g->desc_layout);
    assert(!err);

    err = vk_create_pool(g->device, &g->pool, .maxSets = 1,
                         .poolSizeCount = countof(counts), .pPoolSizes = counts);
    assert(!err);

    err = vk_alloc_sets(g->device, &g->set, .descriptorPool = g->pool,
                        .descriptorSetCount = 1, .pSetLayouts = &g->desc_layout);
    assert(!err);

    uniform = vk_uniform(g->uniform.buf, uniform_size);
    write = vk_write_uniform(g->set, 0, 1, &uniform);
    vk_write(g->device, 1, &write);

    if (!read_file(&file, "shaders-vk", 0))
        assert(0);
    lengths = data = file.data;

    if (!dp_read(&file, shader_objcount * 2))
        assert(0);

    stage[0] = vk_stage(VK_SHADER_STAGE_VERTEX_BIT);
    stage[1] = vk_stage(VK_SHADER_STAGE_FRAGMENT_BIT);
    stage[2] = vk_stage(VK_SHADER_STAGE_GEOMETRY_BIT);

    pipeline = vk_pipeline(&vi, &ia, &rs, &ds, &cb, &dyns, &ms, &vp, g->pipeline_layout, stage);

    for (i = 0; i < countof(g->pipeline); i++) {
        printf("shader %u\n", i);

        switch (i) {
        case so_post:
        case so_minimap:
            ds = vk_depth_stencil();
            ia = vk_assembly(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
            vi = vk_vertex_input(0, 0, 0, 0);
            break;
        case so_model:
            ds = vk_depth_stencil(.depthTestEnable = 1, .depthWriteEnable = 1,
                                  .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL);
            ia = vk_assembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
            vi = vk_vertex_input(1, vi_bindings, countof(vi_attrs), vi_attrs);
            break;
        case so_2d:
            ds = vk_depth_stencil();
            ia = vk_assembly(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
            vi = vk_vertex_input(1, &vi_bind_16, countof(vi_attr_2d), vi_attr_2d);
            break;
        case so_particle:
            ds = vk_depth_stencil();
            ia = vk_assembly(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
            vi = vk_vertex_input(1, &vi_bind_16, countof(vi_attr_part), vi_attr_part);
            break;
        case so_map:
            ds = vk_depth_stencil(.depthTestEnable = 1, .depthWriteEnable = 1,
                                  .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL);
            ia = vk_assembly(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
            vi = vk_vertex_input(1, &vi_bind_8, countof(vi_attr_map), vi_attr_map);
            break;
        case so_mapx:
            ds = vk_depth_stencil(.depthTestEnable = 1, .depthWriteEnable = 1,
                                  .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL);
            ia = vk_assembly(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
            vi = vk_vertex_input(1, &vi_bind_16, countof(vi_attr_mapx), vi_attr_mapx);
            break;
        default:
            assert(0);
        }
        pipeline.stageCount = (shader_hasgeom & (1 << i)) ? 3 : 2;

        for (j = 0; j < pipeline.stageCount; j++) {
            len = *lengths++;
            p = dp_read(&file, len);
            assert(p);
            err = vk_create_shader(g->device, &stage[j].module, .codeSize = len, .pCode = p);
            assert(!err);
        }

        pipeline.renderPass = g->pass[i == so_post || i == so_2d || i == so_minimap];

        err = vk_create_pipeline(g->device, &g->pipeline[i], &pipeline);
        assert(!err);

        for (j = 0; j < pipeline.stageCount; j++)
            vk_destroy(g->device, stage[j].module);
    }

    free(data);
}

static VkResult set_framebuffers(gfx_t *g, unsigned w, unsigned h)
{
    VkImageView attachments[2];
    VkFramebuffer fb[3];
    VkResult err;

    err = vk_create_fb(g->device, &fb[0], .attachmentCount = 1, .pAttachments = &g->sc_view[0],
                           .width = w, .height = h);
    if (err)
        return err;

    err = vk_create_fb(g->device, &fb[1], .attachmentCount = 1, .pAttachments = &g->sc_view[1],
                           .width = w, .height = h);
    if (err)
        goto fail_destroy0;

    attachments[0] = g->render_tex.view;
    attachments[1] = g->depth.view;

    err = vk_create_fb(g->device, &fb[2], .attachmentCount = 2, .pAttachments = attachments,
                       .width = w, .height = h);
    if (err)
        goto fail_destroy1;

    g->fb[0] = fb[0];
    g->fb[1] = fb[1];
    g->fb[2] = fb[2];
    return 0;

fail_destroy0:
    vk_destroy(g->device, g->fb[0]);
fail_destroy1:
    vk_destroy(g->device, g->fb[1]);
    return err;
}

bool gfx_init(gfx_t *g, unsigned w, unsigned h)
{
    static const char* ext[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN_SURFACE_EXTENSION_NAME};
    static const char* sc_ext[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    static const float queue_priority = 0.0;
    static const VkApplicationInfo app = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "test",
        .applicationVersion = 0,
        .pEngineName = "test",
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION,
    };

    VkResult err;
    VkDeviceQueueCreateInfo queue;
    VkFormatProperties props;
    VkPhysicalDevice gpu;
    uint32_t count, queue_index;
    //unsigned i;

    /* */
    err = vk_create_instance(&g->inst,
                             .pApplicationInfo = &app,
                             .enabledExtensionCount = countof(ext),
                             .ppEnabledExtensionNames = ext);
    assert(!err);

    /* gpu choice: take first gpu */
    count = 1;
    err = vk_enum_gpu(g->inst, &count, &gpu);
    assert(!err && count);

    vkGetPhysicalDeviceFormatProperties(gpu, rgba_format, &props);
    assert(props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

    vkGetPhysicalDeviceFormatProperties(gpu, alpha_format, &props);
    assert(props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

    /* */
    err = create_window(w, h, g->inst, &g->surface);
    assert(!err);

    /* */
    queue_index = 0; //TODO find first queue with required features
    queue = vk_queue_info(queue_index, &queue_priority);

    /* */
    err = vk_create_device(gpu, &g->device,
                           .queueCreateInfoCount = 1,
                           .pQueueCreateInfos = &queue,
                           .enabledExtensionCount = 1,
                           .ppEnabledExtensionNames = sc_ext);
    assert(!err);

    vk_get_queue(g->device, queue_index, &g->queue);

    /* format choice */
    count = 1;
    err = vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, g->surface, &count, &g->fmt);
    assert((!err || err == VK_INCOMPLETE) && count);

    if (g->fmt.format == VK_FORMAT_UNDEFINED)
        g->fmt.format = VK_FORMAT_B8G8R8A8_UNORM;

    /* */
    vk_mem_props(gpu, &g->mem_props);

    err = render_pass(g->device, &g->pass[0], g->fmt.format, depth_format);
    assert(!err);

    err = render_pass(g->device, &g->pass[1], g->fmt.format, 0);
    assert(!err);

    err = vk_create_sampler(g->device, &g->sampler[0]);
    assert(!err);

    err = vk_create_sampler(g->device, &g->sampler[1], .minFilter = VK_FILTER_LINEAR);
    assert(!err);

    err = create_buffer(g, &g->uniform, uniform_size,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    assert(!err);

    err = create_buffer(g, &g->verts2d, 16 * 4096, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    assert(!err);

    err = create_buffer(g, &g->particles, 16 * 4096, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    assert(!err);

    err = create_buffer(g, &g->vbo[vo_sel], 16 * 4096, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    assert(!err);

    pipelines(g); //TODO

    err = vk_create_cmdpool(g->device, &g->cmd_pool, .queueFamilyIndex = queue_index);
    assert(!err);

    err = vk_create_cmd(g->device, &g->cmd, .commandPool = g->cmd_pool, .commandBufferCount = 1);
    assert(!err);

    err = set_swapchain(g, w, h);
    assert(!err);

    err = set_texture(g, &g->depth, w, h, 1, format_depth_render);
    assert(!err);

    err = set_texture(g, &g->render_tex, w, h, 1, format_rgba_render);
    assert(!err);

    err = set_framebuffers(g, w, h);
    assert(!err);

    VkDescriptorImageInfo sampler;
    VkWriteDescriptorSet write;

    sampler = vk_sampler(g->sampler[0], g->render_tex.view);
    write = vk_write_sampler(g->set, 3, 0, 1, &sampler);
    vk_write(g->device, 1, &write);

    printf("gfx_init\n");

    g->sc_frame = 0;

    return 1;
}

bool gfx_resize(gfx_t *g, unsigned w, unsigned h)
{
    VkResult err;

    vk_destroy(g->device, g->fb[0]);
    vk_destroy(g->device, g->fb[1]);
    vk_destroy(g->device, g->fb[2]);

    err = set_swapchain(g, w, h);
    assert(!err);

    err = set_texture(g, &g->depth, w, h, 1, format_depth_render);
    assert(!err);

    err = set_texture(g, &g->render_tex, w, h, 1, format_rgba_render);
    assert(!err);

    err = set_framebuffers(g, w, h);
    assert(!err);

    VkDescriptorImageInfo sampler;
    VkWriteDescriptorSet write;

    sampler = vk_sampler(g->sampler[0], g->render_tex.view);
    write = vk_write_sampler(g->set, 3, 0, 1, &sampler);
    vk_write(g->device, 1, &write);

    g->w = w;
    g->h = h;

    printf("resize\n");

    g->sc_frame = 0;

    return 1;
}

void gfx_done(gfx_t *g)
{
    vk_destroy(g->device, g->sampler[0]);
    vk_destroy(g->device, g->sampler[1]);
    destroy_swapchain(g);
    vk_destroy(g->device, g->pass[0]);
    vk_destroy(g->device, g->pass[1]);
    vk_destroy(g->device);
    destroy_window(g->inst, g->surface);
    vk_destroy(g->inst);
}

static const VkClearValue clear_values[2] = {
    {.color.float32 = {0.2, 0.2, 0.2, 0.2}},
    {.depthStencil = {1.0, 0.0}},
};

static void write_sampler(gfx_t *g, VkImageView view, unsigned binding, unsigned filter)
{
    VkDescriptorImageInfo sampler;
    VkWriteDescriptorSet write;

    sampler = vk_sampler(g->sampler[filter], view);
    write = vk_write_sampler(g->set, binding, 0, 1, &sampler);
    vk_write(g->device, 1, &write);
}

void gfx_render_minimap(gfx_t *g, unsigned w)
{
    VkResult err;

    if (g->fb[3])
        vk_destroy(g->device, g->fb[3]);

    err = set_texture(g, &g->tex[tex_minimap], w, w, 1, format_rgba_render);
    assert(!err);

    write_sampler(g, g->tex[tex_minimap].view, 8, filter_none);

    err = vk_create_fb(g->device, &g->fb[3], .attachmentCount = 1,
                       .pAttachments = &g->tex[tex_minimap].view, .width = w, .height = w);
    assert(!err);

    err = vk_begin_cmd(g->cmd);
    assert(!err);

    vk_set_scissor(g->cmd, .extent.width = w, .extent.height = w);
    vk_set_viewport(g->cmd, .width = w, .height = w);

    vk_begin_render(g->cmd,
        .renderPass = g->pass[1],
        .framebuffer = g->fb[3],
        .renderArea.extent.width = w,
        .renderArea.extent.height = w,
        .clearValueCount = 1,
        .pClearValues = clear_values);

    vk_bind_pipeline(g->cmd, g->pipeline[so_minimap]);
    vk_bind_descriptors(g->cmd, g->pipeline_layout, 1, &g->set);

    vk_draw(g->cmd, 1, 0);

    vk_end_render(g->cmd);

    /*vk_image_barrier(g->cmd, .image = g->tex[tex_minimap].image,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);*/

    err = vk_end_cmd(g->cmd);
    assert(!err);

    VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                .commandBufferCount = 1,
                                .pCommandBuffers = &g->cmd};

    err = vkQueueSubmit(g->queue, 1, &submit_info, 0);
    assert(!err);

    err = vkQueueWaitIdle(g->queue);
    assert(!err);
}

void gfx_mapverts(gfx_t *g, mapvert_t *vert, uint32_t nvert)
{
    VkResult err;
    buffer_t *buf;

    buf = &g->vbo[vo_map];
    assert(!buf->mem);
    err = create_buffer(g, buf, nvert * sizeof(mapvert_t), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    assert(!err);

    vk_memcpy(g->device, buf->mem, 0, vert, nvert * sizeof(mapvert_t));
}

void gfx_map_begin(gfx_t *g)
{
    vk_bind_pipeline(g->cmd, g->pipeline[so_map]);
    vk_bind_descriptors(g->cmd, g->pipeline_layout, 1, &g->set);

    vk_bind_buf(g->cmd, &g->vbo[vo_map].buf, 0);
}

void gfx_map_draw(gfx_t *g, uint32_t offset, uint32_t count)
{
    vk_draw(g->cmd, count, offset);
}

void gfx_mdl_outline(gfx_t *g, vec3 color)
{
    (void) g;
    (void) color;
}

void gfx_mdl_outline_off(gfx_t *g)
{
    (void) g;
}

void gfx_begin_draw(gfx_t *g, uint32_t w, uint32_t h, float *matrix,
                    mapvert2_t *mv, unsigned mcount,
                    psystem_t *p, vert2d_t *v, unsigned count)
{
    VkResult err;
    void *data;
    float *pf;

    vkDeviceWaitIdle(g->device);

    err = vk_map(g->device, g->uniform.mem, &data);
    assert(!err);

    memcpy(data, matrix, 4 * 16);

    pf = data;
    pf[16] = 2.0 / w;
    pf[17] = 2.0 / h;
    pf[18] = h / 1080.0;

    vk_unmap(g->device, g->uniform.mem);

    assert(count <= 4096);
    vk_memcpy(g->device, g->verts2d.mem, 0, v, count * sizeof(*v));

    assert(p->count <= 4096);
    vk_memcpy(g->device, g->particles.mem, 0, p->data, p->count * sizeof(*p->data));

    assert(p->count <= 4096);
    vk_memcpy(g->device, g->vbo[vo_sel].mem, 0, mv, mcount * sizeof(*mv));

    g->w = w;
    g->h = h;
    g->vert2d_count = count;
    g->sel_count = mcount;
    g->particle_count = p->count;

    err = vk_begin_cmd(g->cmd);
    assert(!err);

    vk_set_scissor(g->cmd, .extent.width = w, .extent.height = h);
    vk_set_viewport(g->cmd, .width = w, .height = h);

    vk_begin_render(g->cmd,
        .renderPass = g->pass[0],
        .framebuffer = g->fb[2],
        .renderArea.extent.width = w,
        .renderArea.extent.height = h,
        .clearValueCount = 2,
        .pClearValues = clear_values);
}

void gfx_finish_draw(gfx_t *g)
{
    VkResult err;
    VkSemaphore semaphore;
    VkCommandBuffer cmd;
    unsigned frame;

    cmd = g->cmd;

    vk_bind_pipeline(g->cmd, g->pipeline[so_mapx]);
    vk_bind_descriptors(g->cmd, g->pipeline_layout, 1, &g->set);

    vk_bind_buf(cmd, &g->vbo[vo_sel].buf, 0);
    if (g->sel_count)
        vk_draw(cmd, g->sel_count, 0);

    vk_bind_pipeline(g->cmd, g->pipeline[so_particle]);
    vk_bind_descriptors(g->cmd, g->pipeline_layout, 1, &g->set);

    vk_bind_buf(cmd, &g->particles.buf, 0);
    if (g->particle_count)
        vk_draw(cmd, g->particle_count, 0);

    vk_end_render(g->cmd);

    vk_image_barrier(g->cmd, .image = g->render_tex.image,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    vk_begin_render(g->cmd,
        .renderPass = g->pass[1],
        .framebuffer = g->fb[g->sc_frame],
        .renderArea.extent.width = g->w,
        .renderArea.extent.height = g->h,
        .clearValueCount = 1,
        .pClearValues = clear_values);

    vk_bind_pipeline(g->cmd, g->pipeline[so_post]);
    vk_bind_descriptors(g->cmd, g->pipeline_layout, 1, &g->set);

    vk_draw(g->cmd, 1, 0);

    vk_bind_pipeline(cmd, g->pipeline[so_2d]);
    vk_bind_descriptors(cmd, g->pipeline_layout, 1, &g->set);

    vk_bind_buf(cmd, &g->verts2d.buf, 0);
    vk_draw(cmd, g->vert2d_count, 0);

    vk_end_render(cmd);

    vk_image_barrier(cmd, .image = g->sc_image[g->sc_frame],
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    err = vk_end_cmd(cmd);
    assert(!err);

    err = vk_create_semaphore(g->device, &semaphore);
    assert(!err);

    //TODO what is semaphore for?
    err = vk_next_image(g->device, g->swapchain, semaphore, &frame);
    assert(!err);

    assert(frame == g->sc_frame);

    VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                .waitSemaphoreCount = 1,
                                .pWaitSemaphores = &semaphore,
                                .pWaitDstStageMask = &pipe_stage_flags,
                                .commandBufferCount = 1,
                                .pCommandBuffers = &g->cmd,
                                .signalSemaphoreCount = 0,
                                .pSignalSemaphores = NULL};

    err = vkQueueSubmit(g->queue, 1, &submit_info, 0);
    assert(!err);

    VkPresentInfoKHR present = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .swapchainCount = 1,
        .pSwapchains = &g->swapchain,
        .pImageIndices = &g->sc_frame,
    };

    // TBD/TODO: SHOULD THE "present" PARAMETER BE "const" IN THE HEADER?
    err = vkQueuePresentKHR(g->queue, &present);
    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        // demo->swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
        //demo_resize(demo);
    } else if (err == VK_SUBOPTIMAL_KHR) {
        printf("suboptimal\n");
        // demo->swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
    } else {
        assert(!err);
    }

    err = vkQueueWaitIdle(g->queue);
    assert(err == VK_SUCCESS);

    vk_destroy(g->device, semaphore);

    vkDeviceWaitIdle(g->device);

    g->sc_frame = !g->sc_frame;
}

void gfx_mdl_begin(gfx_t *g)
{
    vk_bind_pipeline(g->cmd, g->pipeline[so_model]);
    vk_bind_descriptors(g->cmd, g->pipeline_layout, 1, &g->set);
}

void gfx_mdl(gfx_t *g, unsigned id, float *matrix)
{
    vk_bind_buf(g->cmd, &g->vbo[vo_mdl + id].buf, 0);
    vk_bind_index_buf(g->cmd, g->vbo[vo_mdl_elements + id].buf, 0);

    vk_image_barrier(g->cmd,
        .image = g->render_tex.image,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT);

    vkCmdUpdateBuffer(g->cmd, g->uniform.buf, uniform_offset(m0), 4 * 16, (void*) matrix);
}

void gfx_mdl_params(gfx_t *g, vec4 *rot, vec4 *tr_sc, uint8_t count, vec4 color)
{
    vkCmdUpdateBuffer(g->cmd, g->uniform.buf, uniform_offset(color), 16, (void*) &color);
    vkCmdUpdateBuffer(g->cmd, g->uniform.buf, uniform_offset(r0), count * 16, (void*) rot);
    vkCmdUpdateBuffer(g->cmd, g->uniform.buf, uniform_offset(d0), count * 16, (void*) tr_sc);

    //barrier needed?
    //vk_buffer_barrier(g->cmd, .buffer = g->uniform.buf, .offset = 0, .size = uniform_size,
    //    .srcAccessMask = ~0u,
    //    .dstAccessMask = ~0u);
}

void gfx_mdl_texture(gfx_t *g, uint16_t tex, vec4 teamcolor)
{
    //barrier?
    struct {
        float texture;
        vec4 team_color;
    } data;

    data.texture = tex & 0x3FFF;
    data.team_color = vec4(0.0, 0.0, 0.0, 1.0);
    if (tex & 0x8000)
        data.team_color = teamcolor;
    else if (tex & 0x4000)
        data.team_color.w = 0.0;

    vkCmdUpdateBuffer(g->cmd, g->uniform.buf, uniform_offset(texture), 20, (void*) &data);
}

void gfx_mdl_draw(gfx_t *g, uint32_t start, uint32_t end)
{
    vkCmdDrawIndexed(g->cmd, end - start, 1, start, 0, 0);
}

void gfx_texture(gfx_t *g, unsigned id, unsigned width, unsigned height, unsigned layers,
                 unsigned filter, unsigned mipmap, unsigned format)
{
    (void) mipmap;

    texture_t *tex;
    VkResult err;
    unsigned binding;

    VkDescriptorImageInfo sampler;
    VkWriteDescriptorSet write;


    tex = &g->tex[id];

    err = set_texture(g, tex, width, height, layers ? layers : 1, format);
    assert(!err);

    binding = (id == tex_tiles) ? 6 :
              (id == tex_icons) ? 2 :
              (id == tex_part) ? 5 :
              (id == tex_text) ? 1 :
              (id == tex_map) ? 7 :
              (id == tex_mdl) ? 4 :
              (assert(0), 1);

    sampler = vk_sampler(g->sampler[filter], tex->view);
    write = vk_write_sampler(g->set, binding, 0, 1, &sampler);
    vk_write(g->device, 1, &write);
}

void* gfx_map(gfx_t *g, unsigned id)
{
    VkResult err;
    void *res;

    err = vk_map(g->device, g->tex[id].mem, &res);
    assert(!err);

    return res;
}

void gfx_unmap(gfx_t *g, unsigned id)
{
    vk_unmap(g->device, g->tex[id].mem);
}

void gfx_copy(gfx_t *g, unsigned id, void *data, unsigned size)
{
    void *p;

    p = gfx_map(g, id);
    memcpy(p, data, size);
    gfx_unmap(g, id);
}

void gfx_mdl_data(gfx_t *g, unsigned id, const uint16_t *index, const void *vert,
                 unsigned nindex, unsigned nvert)
{
    VkResult err;
    buffer_t *buf;

    buf = &g->vbo[vo_mdl + id];
    assert(!buf->mem);
    err = create_buffer(g, buf, nvert * vert_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    assert(!err);

    err = vk_memcpy(g->device, buf->mem, 0, vert, nvert * vert_size);
    assert(!err);

    buf = &g->vbo[vo_mdl_elements + id];
    assert(!buf->mem);
    err = create_buffer(g, buf, nindex * sizeof(*index), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    assert(!err);

    err = vk_memcpy(g->device, buf->mem, 0, index, nindex * sizeof(*index));
    assert(!err);
}

void gfx_particleinfo(gfx_t *g, uint32_t *info, uint32_t count)
{
    vk_memcpy(g->device, g->uniform.mem, uniform_offset(info), info, count * 16);
}

#endif
