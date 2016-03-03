#include "gfx.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "resource.h"

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

#if 0 //TODO
#define glUniform1fv glUniform1dv
#define glUniform2fv glUniform2dv
#define glUniform3fv glUniform3dv
#define glUniform4fv glUniform4dv
#define glUniformMatrix4fv glUniformMatrix4dv
#endif

typedef struct {
    const char *vert, *geom, *frag;
} shaderdata_t;

extern const shaderdata_t shaderdata[];

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

static bool shader_load(shader_t *res, shaderdata_t data)
{
    GLuint prog, vert, geom, frag;
    GLint status;

    vert = glCreateShader(GL_VERTEX_SHADER);
    if (!vert)
        return 0;

    if (!compile_shader(vert, data.vert))
        goto EXIT_DELETE_VERT;

    frag = glCreateShader(GL_FRAGMENT_SHADER);
    if (!frag)
        goto EXIT_DELETE_VERT;

    if (!compile_shader(frag, data.frag))
        goto EXIT_DELETE_FRAG;

    geom = 0;
    if (data.geom) {
        geom = glCreateShader(GL_GEOMETRY_SHADER);
        if (!geom)
            goto EXIT_DELETE_FRAG;

        if (!compile_shader(geom, data.geom))
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

bool gfx_init(gfx_t *g, int w, int h)
{
    unsigned i;
    const shader_t *s;

    if (!create_window(w, h))
        return 0;

    glDepthFunc(GL_LEQUAL);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    for (i = 0; i < num_shader; i++)
        if (!shader_load(&g->shader[i], shaderdata[i]))
            goto fail;

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

    s = &g->shader[so_mdl];
    g->uniform_outline = glGetUniformLocation(s->prog, "outline");
    g->uniform_rot = glGetUniformLocation(s->prog, "r");
    g->uniform_trans = glGetUniformLocation(s->prog, "d");

    glGenBuffers(num_vbo, g->vbo);
    glGenVertexArrays(num_vao, g->vao);
    glGenTextures(num_tex, g->tex);
    glGenFramebuffers(num_fbo, g->fbo);
    glGenRenderbuffers(1, &g->depth);

    /* default texture parameters */
    for (i = 0; i < countof(g->tex); i++) {
        glBindTexture(GL_TEXTURE_2D, g->tex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

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
    gfx_texture_data(g, tex_screen0, 0, w, h, 0, 0, 0);
    gfx_texture_data(g, tex_screen1, 0, w, h, 0, 0, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, g->depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, w, h);

    gfx_texture_data(g, tex_minimap, 0, 1024, 1024, 0, 0, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, g->fbo[1]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g->tex[tex_minimap], 0);

    glBindFramebuffer(GL_FRAMEBUFFER, g->fbo[0]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g->tex[tex_screen0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, g->tex[tex_screen1], 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, g->depth);

    return glGetError() == 0;

fail:
    //TODO
    return 0;
}

bool gfx_resize(gfx_t *g, int w, int h)
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
    int i;

    for (i = 0; i < num_shader; i++)
        glDeleteProgram(g->shader[i].prog);

    glDeleteBuffers(num_vbo, g->vbo);
    glDeleteVertexArrays(num_vao, g->vao);
    glDeleteTextures(num_tex, g->tex);
    glDeleteFramebuffers(num_fbo, g->fbo);
    glDeleteRenderbuffers(1, &g->depth);

    destroy_window();
}

void gfx_render_minimap(gfx_t *g, int w)
{
    const shader_t *s;
    float scale;

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
    glBindVertexArray(g->vao[vo_null]);
    glDrawArrays(GL_POINTS, 0, 1);

    glEnable(GL_BLEND);
}

void gfx_mdl_data(gfx_t *g, size_t id, const uint16_t *index, const void *vert,
                 size_t nindex, size_t nvert)
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

    s = &g->shader[so_part];
    glUseProgram(s->prog);
    glUniform4uiv(s->var1, count, info);
}

void gfx_mapverts(gfx_t *g, mapvert_t *vert, uint32_t nvert)
{
    glBindBuffer(GL_ARRAY_BUFFER, g->vbo[vo_map]);
    glBufferData(GL_ARRAY_BUFFER, nvert * sizeof(mapvert_t), vert, GL_STATIC_DRAW);
}

void gfx_begin_draw(gfx_t *g, uint32_t w, uint32_t h, float *matrix,
                    mapvert2_t *mv, size_t mcount,
                    psystem_t *p, vert2d_t *v, size_t count)
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

    s = &g->shader[so_sel];
    glUseProgram(s->prog);
    glUniformMatrix4fv(s->matrix, 1, GL_FALSE, g->matrix);
    glBindVertexArray(g->vao[vo_sel]);
    glDrawArrays(GL_POINTS, 0, g->sel_count);

    glDisable(GL_DEPTH_TEST);

    sc = (float) g->h / g->w;

    s = &g->shader[so_part];
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
    glBindVertexArray(g->vao[vo_null]);
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
    glUseProgram(g->shader[so_mdl].prog);
}

void gfx_mdl(gfx_t *g, size_t id, float *matrix)
{
    glBindVertexArray(g->vao[vo_mdl + id]);
    glUniformMatrix4fv(g->shader[so_mdl].matrix, 1, GL_FALSE, matrix);
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

    glBindTexture(GL_TEXTURE_2D, 0);
    glUniform4fv(g->shader[so_mdl].var0, 1, &color.x);
}

void gfx_mdl_texture(gfx_t *g, uint16_t tex, vec4 teamcolor)
{
    const float v[5] = {0.0, 0.0, 0.0, 0.0, 1.0};

    glUniform4fv(g->shader[so_mdl].var1, 1, (tex&0x8000) ? &teamcolor.x : ((tex&0x4000) ? v : v + 1));

    tex &= 0x3FFF;
    glBindTexture(GL_TEXTURE_2D, tex == 0x3FFF ? 0 : g->tex[tex_res + tex]);
}

void gfx_mdl_draw(gfx_t *g, uint32_t start, uint32_t end)
{
    (void) g;
    glDrawElements(GL_TRIANGLES, end - start, GL_UNSIGNED_SHORT, (void*)(size_t)(start * 2));
}

void gfx_texture_data(gfx_t *g, size_t id, void *data, size_t width, size_t height,
                      uint8_t filter, uint8_t mipmap, uint8_t fmt)
{
    (void) filter;

    static const GLint swizzle[] = {GL_ONE, GL_ONE, GL_ONE, GL_RED};
    int format, internal;

    format = (fmt == 2) ? GL_RG_INTEGER : fmt ? GL_RED : GL_RGBA;
    internal = (fmt == 2) ? GL_RG8UI : format;

    glBindTexture(GL_TEXTURE_2D, g->tex[id]);
    if (fmt == 1)
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
    if (mipmap) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipmap);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, internal, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    if (mipmap)
        glGenerateMipmap(GL_TEXTURE_2D);
}

void gfx_texture_mdl(gfx_t *g, size_t id, void *data, size_t width, size_t height)
{
    gfx_texture_data(g, tex_res + id, data, width, height, 0, 0, 0);
}

#else

VkResult create_window(int width, int height, VkInstance inst, VkSurfaceKHR *surface);
void destroy_window(VkInstance inst, VkSurfaceKHR surface);

static void init_swapchain(gfx_t *g, uint32_t w, uint32_t h) {
    uint32_t i, count;
    VkResult err;
    VkSwapchainKHR swapchain;

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
        .oldSwapchain = g->swapchain,
        .clipped = 1,
    );
    assert(!err);

    if (g->swapchain)
        vk_destroy_swapchain(g->device, g->swapchain);

    g->swapchain = swapchain;

    count = 2;
    err = vk_get_images(g->device, g->swapchain, &count, g->sc_image);
    assert(!err && count == 2);

    for (i = 0; i < count; i++) {
        err = vk_create_view(g->device, &g->sc_view[i],
                            .format = g->fmt.format,
                            .image = g->sc_image[i]);
        assert(!err);
    }
}

VkResult allocate(gfx_t *g, VkDeviceMemory *res, VkMemoryRequirements *reqs,
                  bool visible)
{
    uint32_t bits;
    unsigned i;

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

VkResult create_image(gfx_t *g, VkImage *res_image, VkDeviceMemory *res_mem,
                      uint32_t width, uint32_t height, uint32_t layers, VkFormat format,
                      VkImageTiling tiling, VkImageUsageFlags usage, bool visible)
{
    VkResult err;
    VkImage image;
    VkMemoryRequirements mem_reqs;
    VkDeviceMemory mem;

    err = vk_create_image(g->device, &image,
                         .imageType = VK_IMAGE_TYPE_2D,
                         .format = format,
                         .extent = {width, height, 1},
                         .mipLevels = 1,
                         .arrayLayers = layers,
                         .samples = VK_SAMPLE_COUNT_1_BIT,
                         .tiling = tiling,
                         .usage = usage);
    assert(!err);

    vk_mem_reqs(g->device, image, &mem_reqs);

    err = allocate(g, &mem, &mem_reqs, visible);
    assert(!err);

    err = vk_bind(g->device, image, mem);
    assert(!err);

    *res_image = image;
    *res_mem = mem;
    return 0;
}

void depth(gfx_t *g, uint32_t w, uint32_t h)
{
    VkResult err;

    err = create_image(g, &g->depth_image, &g->depth_mem, w, h, 1, depth_format,
                 VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 0);
    assert(!err);

    err = vk_create_view(g->device, &g->depth_view,
                   .image = g->depth_image,
                   .format = depth_format,
                   .subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT);
    assert(!err);
}

VkResult render_pass(VkDevice dev, VkRenderPass *res, VkFormat format, VkFormat depth)
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

VkResult create_buffer(gfx_t *g, buffer_t *res, VkDeviceSize size, VkBufferUsageFlags usage)
{
    VkResult err;
    VkBuffer buf;
    VkMemoryRequirements mem_reqs;
    VkDeviceMemory mem;

    err = vk_create_buffer(g->device, &buf, .size = size, .usage = usage);
    if (err)
        goto fail;

    vk_mem_reqs(g->device, buf, &mem_reqs);

    err = allocate(g, &mem, &mem_reqs, 1);
    if (err)
        goto fail_destroy_buffer;

    err = vk_bind(g->device, buf, mem);
    if (err)
        goto fail_free;

    res->buf = buf;
    res->mem = mem;
    return 0;

fail_free:
    vk_free(g->device, mem);
fail_destroy_buffer:
    vk_destroy_buffer(g->device, buf);
fail:
    return err;
}

void create_texture(gfx_t *g, uint32_t width, uint32_t height, texture_t *tex)
{
    VkImage image;
    VkDeviceMemory mem;
    VkImageView view;

    VkImageUsageFlags usage;
    VkResult err;

    usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    create_image(g, &image, &mem, width, height, 1, tex_format, VK_IMAGE_TILING_LINEAR, usage, 0);

    err = vk_create_view(g->device, &view, .image = image, .format = tex_format);
    assert(!err);

    tex->image = image;
    tex->mem = mem;
    tex->view = view;
}

void databuffers(gfx_t *g)
{
    VkResult err;

    err = create_buffer(g, &g->uniform, uniform_size,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    assert(!err);

    err = create_buffer(g, &g->verts2d, 16 * 4096, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    assert(!err);

    err = create_buffer(g, &g->particles, 16 * 4096, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    assert(!err);

    err = create_buffer(g, &g->vbo[vo_sel], 16 * 4096, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    assert(!err);
}

void pipelines(gfx_t *g) {
    //TODO cant render without SCISSOR??
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
    file_t file;
    void *data, *p;
    uint16_t *lengths;
    unsigned i, j;
    size_t len;

    VkDescriptorSetLayoutBinding layout[9];
    VkDescriptorImageInfo sampler[3];
    VkDescriptorBufferInfo uniform;
    VkWriteDescriptorSet writes[4];

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
    sampler[0] = vk_sampler(g->sampler[0], g->render_tex.view);
    sampler[1] = vk_sampler(g->sampler[0], g->model_tex.view);
    sampler[2] = vk_sampler(g->sampler[0], g->tex[tex_minimap].view);

    writes[0] = vk_write_uniform(g->set, 0, 1, &uniform);
    writes[1] = vk_write_sampler(g->set, 3, 0, 1, &sampler[0]);
    writes[2] = vk_write_sampler(g->set, 4, 0, 1, &sampler[1]);
    writes[3] = vk_write_sampler(g->set, 8, 0, 1, &sampler[2]);

    vk_write(g->device, countof(writes), writes);


    if (!read_file(&file, "shaders-vk", 0))
        assert(0);
    lengths = data = file.data;

    if (!dp_read(&file, shader_objcount * 2))
        assert(0);

    stage[0] = vk_stage(VK_SHADER_STAGE_VERTEX_BIT);
    stage[1] = vk_stage(VK_SHADER_STAGE_FRAGMENT_BIT);
    stage[2] = vk_stage(VK_SHADER_STAGE_GEOMETRY_BIT);

    pipeline = vk_pipeline(&vi, &ia, &rs, &ds, &cb, &dyns, &ms, &vp, g->pipeline_layout, stage);

    for (i = 0; i < so_max; i++) {
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

    file.data = data;
    close_file(&file);
}

void framebuffers(gfx_t *g, uint32_t w, uint32_t h)
{
    VkImageView attachments[2];
    VkResult err;
    unsigned i;

    attachments[0] = g->render_tex.view;
    attachments[1] = g->depth_view;

    err = vk_create_fb(g->device, &g->fb[2], .attachmentCount = 2, .pAttachments = attachments,
                       .width = w, .height = h);
    assert(!err);

    attachments[0] = g->tex[tex_minimap].view;

    err = vk_create_fb(g->device, &g->fb[3], .attachmentCount = 1, .pAttachments = attachments,
                   .width = 1024, .height = 1024);
    assert(!err);

    for (i = 0; i < 2; i++) {
        attachments[0] = g->sc_view[i];
        err = vk_create_fb(g->device, &g->fb[i], .attachmentCount = 1, .pAttachments = attachments,
                       .width = w, .height = h);
        assert(!err);
    }
}

#include <stdlib.h>

bool gfx_init(gfx_t *g, int w, int h)
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
    err = vk_enum_gpu(g->inst, &count, &g->gpu);
    assert(!err && count);

    vkGetPhysicalDeviceFormatProperties(g->gpu, tex_format, &props);
    assert(props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

    /* */
    err = create_window(w, h, g->inst, &g->surface);
    assert(!err);

    /* */
    queue_index = 0; //TODO find first queue with required features
    queue = vk_queue_info(queue_index, &queue_priority);

    /* */
    err = vk_create_device(g->gpu, &g->device,
                           .queueCreateInfoCount = 1,
                           .pQueueCreateInfos = &queue,
                           .enabledExtensionCount = 1,
                           .ppEnabledExtensionNames = sc_ext);
    assert(!err);


    vk_get_queue(g->device, queue_index, &g->queue);

    /* format choice */
    count = 1;
    err = vkGetPhysicalDeviceSurfaceFormatsKHR(g->gpu, g->surface, &count, &g->fmt);
    assert((!err || err == VK_INCOMPLETE) && count);

    if (g->fmt.format == VK_FORMAT_UNDEFINED)
        g->fmt.format = VK_FORMAT_B8G8R8A8_UNORM;

    /* */
    vk_mem_props(g->gpu, &g->mem_props);

    err = render_pass(g->device, &g->pass[0], g->fmt.format, depth_format);
    assert(!err);

    err = render_pass(g->device, &g->pass[1], g->fmt.format, 0);
    assert(!err);

    init_swapchain(g, w, h);
    depth(g, w, h);

    err = vk_create_sampler(g->device, &g->sampler[0]);
    assert(!err);

    err = vk_create_sampler(g->device, &g->sampler[1], .minFilter = VK_FILTER_LINEAR);
    assert(!err);

    create_texture(g, w, h, &g->render_tex);

    create_texture(g, 1024, 1024, &g->tex[tex_minimap]);

    create_image(g, &g->model_tex.image, &g->model_tex.mem, 256, 256, 256, tex_format,
                 VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT, 1);

    err = vk_create_view(g->device, &g->model_tex.view, .image = g->model_tex.image, .format = tex_format);
    assert(!err);

    databuffers(g);

    pipelines(g);

    framebuffers(g, w, h);

    err = vk_create_cmdpool(g->device, &g->cmd_pool, .queueFamilyIndex = queue_index);
    assert(!err);

    err = vk_create_cmd(g->device, &g->cmd, .commandPool = g->cmd_pool, .commandBufferCount = 1);
    assert(!err);

    printf("gfx_init\n");

    g->sc_curr = 0;

    return 1;
}

bool gfx_resize(gfx_t *g, int w, int h)
{
    return 1;
}

void gfx_done(gfx_t *g)
{
}

static const VkClearValue clear_values[2] = {
    {.color.float32 = {0.2, 0.2, 0.2, 0.2}},
    {.depthStencil = {1.0, 0.0}},
};

void gfx_render_minimap(gfx_t *g, int w)
{
    VkResult err;

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
}

void gfx_mdl_outline_off(gfx_t *g)
{
}

/*
const shader_t *s;

    s = &g->shader[so_sel];

    glBindVertexArray(g->vao[vo_sel]);
    glDrawArrays(GL_POINTS, 0, count);
*/

void gfx_begin_draw(gfx_t *g, uint32_t w, uint32_t h, float *matrix,
                    mapvert2_t *mv, size_t mcount,
                    psystem_t *p, vert2d_t *v, size_t count)
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

    err = vk_create_semaphore(g->device, &semaphore);
    assert(!err);

    err = vkAcquireNextImageKHR(g->device, g->swapchain, UINT64_MAX, semaphore, 0, &g->sc_curr);
    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
        printf("out of date\n");
        vk_destroy(g->device, semaphore);
        return;
    } else if (err == VK_SUBOPTIMAL_KHR) {
        assert(0);
    } else {
        assert(!err);
    }

    vk_begin_render(g->cmd,
        .renderPass = g->pass[1],
        .framebuffer = g->fb[g->sc_curr],
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

    vk_image_barrier(cmd, .image = g->sc_image[g->sc_curr],
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    err = vk_end_cmd(cmd);
    assert(!err);

    VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                .pNext = NULL,
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
        .pNext = NULL,
        .swapchainCount = 1,
        .pSwapchains = &g->swapchain,
        .pImageIndices = &g->sc_curr,
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
}

void gfx_mdl_begin(gfx_t *g)
{
    vk_bind_pipeline(g->cmd, g->pipeline[so_model]);
    vk_bind_descriptors(g->cmd, g->pipeline_layout, 1, &g->set);
}

void gfx_mdl(gfx_t *g, size_t id, float *matrix)
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

void gfx_texture_data(gfx_t *g, size_t id, void *data, size_t width, size_t height,
                      uint8_t filter, uint8_t mipmap, uint8_t format)
{
    (void) filter;
    (void) mipmap;

    texture_t *tex;
    void *tdata;
    VkResult err;
    VkFormat fmt;

    assert(id < countof(g->tex));
    tex = &g->tex[id];

    if (!tex->mem) {
        fmt = (format == 2) ? VK_FORMAT_R8G8B8A8_UINT : VK_FORMAT_R8G8B8A8_UNORM;
        create_image(g, &tex->image, &tex->mem, width, height, 1, fmt,
                 VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT, 1);
    }

    if (data) {
        err = vk_map(g->device, tex->mem, &tdata);
        assert(!err);

        if (format) {
            uint8_t *p = tdata, *q = data;
            for (unsigned y = 0; y < height; y++) {
                for (unsigned x = 0; x < width; x++, p += 4) {
                    if (format == 1) {
                        p[0] = p[1] = p[2] = 255, p[3] = q[y * width + x];
                    } else {
                        p[0] = q[(y * width + x) * 2 + 0];
                        p[1] = q[(y * width + x) * 2 + 1];
                        p[2] = p[3] = 255;
                    }
                }
            }
        } else {
            memcpy(tdata, data, width * height * 4);
        }

        vk_unmap(g->device, tex->mem);
    }

    if (!tex->view) {
    /*if (gray) {
        err = vk_create_view(g->device, &tex->view, .image = tex->image, .format = tex_format,
                             .components = { VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_ONE,
                                             VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_R, });
        assert(!err);

    } else*/ {
        err = vk_create_view(g->device, &tex->view, .image = tex->image, .format = tex_format);
        assert(!err);
    }
    }

    //
    VkDescriptorImageInfo sampler[2];
    VkWriteDescriptorSet writes[2];

    if (id == tex_icons) {
        sampler[0] = vk_sampler(g->sampler[0], g->tex[tex_text].view);
        sampler[1] = vk_sampler(g->sampler[0], tex->view);

        writes[0] = vk_write_sampler(g->set, 1, 0, 1, sampler);
        writes[1] = vk_write_sampler(g->set, 2, 0, 1, sampler + 1);
        vk_write(g->device, countof(writes), writes);
    }

    if (id == tex_part) {
        sampler[0] = vk_sampler(g->sampler[0], tex->view);
        writes[0] = vk_write_sampler(g->set, 5, 0, 1, sampler);
        vk_write(g->device, 1, writes);
    }

    if (id == tex_tiles) {
        sampler[0] = vk_sampler(g->sampler[0], tex->view);
        writes[0] = vk_write_sampler(g->set, 6, 0, 1, sampler);
        vk_write(g->device, 1, writes);
    }

    if (id == tex_map) {
        sampler[0] = vk_sampler(g->sampler[0], tex->view);
        writes[0] = vk_write_sampler(g->set, 7, 0, 1, sampler);
        vk_write(g->device, 1, writes);
    }
}

void gfx_texture_mdl(gfx_t *g, size_t id, void *data, size_t width, size_t height)
{
    assert(width == 256 && height == 256);
    vk_memcpy(g->device, g->model_tex.mem, id * 256 * 256 * 4, data, 256 * 256 * 4);
}

void gfx_mdl_data(gfx_t *g, size_t id, const uint16_t *index, const void *vert,
                 size_t nindex, size_t nvert)
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
