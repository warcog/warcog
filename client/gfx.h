#ifndef GFX_H
#define GFX_H

#include <stddef.h>
#include <stdbool.h>
#include "math.h"
#include "map.h"
#include "particle.h"

enum { /* enum instead of macro defines */
    num_restex = 1024,
    num_tex = (8 + num_restex),
    num_shader = 8,
    num_fbo = 2,
    num_model = 256,
    num_vbo = (8 + num_model * 2),
    num_vao = (8 + num_model),
    num_text = 2,
};

enum {
    tex_screen0,
    tex_screen1,
    tex_text,
    tex_minimap,
    tex_tiles,
    tex_icons,
    tex_map,
    tex_part,

    tex_res,

};

enum {
    vo_null, //TODO uses no vbo
    vo_2d,
    vo_map,
    vo_sel,
    vo_part,
    vo_mdl = 8,
    vo_mdl_elements = num_model + 8,
};

enum {
    vert_size = 24,
};

#ifndef USE_VULKAN

enum {
    so_post,
    so_2d,
    so_text,
    so_mdl,
    so_map,
    so_sel,
    so_minimap,
    so_part,
};

typedef struct {
    uint32_t prog;
    int32_t matrix, var0, var1;
} shader_t;

typedef struct {
    uint32_t vbo[num_vbo], vao[num_vao], tex[num_tex];
    uint32_t fbo[num_fbo], depth;
    shader_t shader[num_shader];
    int32_t uniform_outline, uniform_rot, uniform_trans;

    uint32_t w, h, vert2d_count, particle_count, sel_count;
    float matrix[16];
} gfx_t;

#else

#include <vk.h>
#include "shaders-vk.h"
#define depth_format VK_FORMAT_D16_UNORM
#define tex_format VK_FORMAT_R8G8B8A8_UNORM

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

typedef struct {
    uint32_t w, h;
    uint32_t sc_curr;
    uint32_t vert2d_count, particle_count, sel_count;

    VkInstance inst;
    VkPhysicalDevice gpu;
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


    VkPipeline pipeline[so_max];
    VkDescriptorSet set;
    VkDescriptorPool pool;

    VkFramebuffer fb[4];

    VkImage depth_image;
    VkDeviceMemory depth_mem;
    VkImageView depth_view;
    texture_t render_tex, model_tex;

    VkSampler sampler[2];

    /* */
    texture_t tex[num_tex];
    buffer_t vbo[num_vbo];
    buffer_t uniform, verts2d, particles;
} gfx_t;

#endif

bool gfx_init(gfx_t *g, int w, int h);
bool gfx_resize(gfx_t *g, int w, int h);
void gfx_done(gfx_t *g);

void gfx_render_minimap(gfx_t *g, int w);
void gfx_particleinfo(gfx_t *g, uint32_t *info, uint32_t count);

void gfx_mapverts(gfx_t *g, mapvert_t *vert, uint32_t nvert);
void gfx_maptiles(gfx_t *g, uint8_t *tile, uint32_t size);

void gfx_begin_draw(gfx_t *g, uint32_t w, uint32_t h, float *matrix,
                    mapvert2_t *mv, size_t mcount,
                    psystem_t *p, vert2d_t *v, size_t count);
void gfx_finish_draw(gfx_t *g);

void gfx_mdl_begin(gfx_t *g);
void gfx_mdl(gfx_t *g, size_t id, float *matrix);
void gfx_mdl_outline(gfx_t *g, vec3 color);
void gfx_mdl_outline_off(gfx_t *g);
void gfx_mdl_params(gfx_t *g, vec4 *rot, vec4 *tr_sc, uint8_t count, vec4 color);
void gfx_mdl_texture(gfx_t *g, uint16_t tex, vec4 teamcolor);
void gfx_mdl_draw(gfx_t *g, uint32_t start, uint32_t end);

void gfx_map_begin(gfx_t *g);
void gfx_map_draw(gfx_t *g, uint32_t offset, uint32_t count);

void gfx_texture_data(gfx_t *g, size_t id, void *data, size_t width, size_t height,
                      uint8_t filter, uint8_t mipmap, uint8_t format);

void gfx_texture_mdl(gfx_t *g, size_t id, void *data, size_t width, size_t height);

void gfx_mdl_data(gfx_t *g, size_t id, const uint16_t *index, const void *vert,
                  size_t nindex, size_t nvert);

//void load_texture(gfx_t *g, size_t id, size_t width, size_t height, void *data, uint8_t filter);
//void load_texture_mipmap(gfx_t *g, size_t id, size_t width, size_t height, void *data);


#endif
