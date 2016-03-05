#ifndef GFX_H
#define GFX_H

#include <stddef.h>
#include <stdbool.h>
#include "math.h"
#include "map.h"
#include "particle.h"
#include "resource.h"

enum {
    vert_size = 24,
};

enum {
    vo_2d,
    vo_map,
    vo_sel,
    vo_part,
    vo_mdl,
    num_vao,
    vo_mdl_elements = vo_mdl + max_models,
    num_vbo = vo_mdl_elements + max_models,
};

enum {
    filter_none,
    filter_linear
};

#ifndef USE_VULKAN

#include "shaders-gl.h"

enum {
    tex_screen0,
    tex_screen1,
    tex_text,
    tex_minimap,
    tex_tiles,
    tex_icons,
    tex_map,
    tex_part,
    tex_mdl,
    num_tex,
};

enum {
    format_rgba,
    format_alpha,
    format_rg_int,
};

typedef struct {
    uint32_t prog;
    int32_t matrix, var0, var1;
} shader_t;

typedef struct {
    unsigned width, height, layers, format;
} texture_t;

typedef struct {
    uint32_t vbo[num_vbo], vao[num_vao + 1], tex[num_tex];
    texture_t texinfo[num_tex];
    uint32_t fbo[2], depth;
    shader_t shader[num_shader];
    int32_t uniform_outline, uniform_rot, uniform_trans, uniform_layer;

    uint32_t w, h, vert2d_count, particle_count, sel_count;
    float matrix[16];

    void *map;
} gfx_t;

#else

#include <vk.h>
#include "shaders-vk.h"

enum {
    tex_text,
    tex_minimap,
    tex_tiles,
    tex_icons,
    tex_map,
    tex_part,
    tex_mdl,
    num_tex
};

enum {
    format_rgba,
    format_alpha,
    format_rg_int,
    format_rgba_render,
    format_depth_render,
};

typedef struct {
    uint32_t w, h;
    uint32_t sc_frame;
    uint32_t vert2d_count, particle_count, sel_count;

    VkInstance inst;
    VkSurfaceKHR surface;
    VkDevice device;

    VkQueue queue;
    VkSurfaceFormatKHR fmt;
    VkPhysicalDeviceMemoryProperties mem_props;

    /* swapchain */
    VkSwapchainKHR swapchain;
    VkImage sc_image[2];
    VkImageView sc_view[2];
    VkCommandBuffer cmd;
    VkCommandPool cmd_pool;

    /* */
    VkRenderPass pass[2];
    VkPipelineLayout pipeline_layout;
    VkDescriptorSetLayout desc_layout;


    VkPipeline pipeline[num_shader];
    VkDescriptorSet set;
    VkDescriptorPool pool;

    VkFramebuffer fb[4];

    texture_t depth;
    texture_t render_tex, model_tex;

    VkSampler sampler[2];

    /* */
    texture_t tex[num_tex];
    buffer_t vbo[num_vbo];
    buffer_t uniform, verts2d, particles;
} gfx_t;

#endif

bool gfx_init(gfx_t *g, unsigned w, unsigned h);
bool gfx_resize(gfx_t *g, unsigned w, unsigned h);
void gfx_done(gfx_t *g);

void gfx_render_minimap(gfx_t *g, unsigned w);

void gfx_particleinfo(gfx_t *g, uint32_t *info, uint32_t count);

void gfx_mapverts(gfx_t *g, mapvert_t *vert, uint32_t nvert);
void gfx_maptiles(gfx_t *g, uint8_t *tile, uint32_t size);

void gfx_begin_draw(gfx_t *g, uint32_t w, uint32_t h, float *matrix,
                    mapvert2_t *mv, unsigned mcount,
                    psystem_t *p, vert2d_t *v, unsigned count);
void gfx_finish_draw(gfx_t *g);

void gfx_mdl_begin(gfx_t *g);
void gfx_mdl(gfx_t *g, unsigned id, float *matrix);
void gfx_mdl_outline(gfx_t *g, vec3 color);
void gfx_mdl_outline_off(gfx_t *g);
void gfx_mdl_params(gfx_t *g, vec4 *rot, vec4 *tr_sc, uint8_t count, vec4 color);
void gfx_mdl_texture(gfx_t *g, uint16_t tex, vec4 teamcolor);
void gfx_mdl_draw(gfx_t *g, uint32_t start, uint32_t end);

void gfx_map_begin(gfx_t *g);
void gfx_map_draw(gfx_t *g, uint32_t offset, uint32_t count);

void gfx_mdl_data(gfx_t *g, unsigned id, const uint16_t *index, const void *vert,
                  unsigned nindex, unsigned nvert);

void gfx_texture(gfx_t *g, unsigned id, unsigned width, unsigned height, unsigned layers,
                 unsigned filter, unsigned mipmap, unsigned format);

void* gfx_map(gfx_t *g, unsigned id);
void gfx_unmap(gfx_t *g, unsigned id);
void gfx_copy(gfx_t *g, unsigned id, void *data, unsigned size);

#endif
