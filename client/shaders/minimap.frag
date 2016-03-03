#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 6) uniform sampler2D tex_map;
layout (binding = 7) uniform isampler2D tex;

layout (location = 0) in vec4 texcoord;

layout (location = 0) out vec4 frag;

void main()
{
    vec2 coord, t0, t1;
    vec4 p0, p1;
    ivec2 tile;

    coord = texcoord.xy;
    tile = texture(tex, coord).xy;
    t0 = vec2(float(tile.x & 15u) + 0.5, float(tile.x >> 4u) + 0.5) * (1.0 / 16.0);
    t1 = vec2(float(tile.y & 15u) + 0.5, float(tile.y >> 4u) + 0.5) * (1.0 / 16.0);
    p0 = texture(tex_map, t0);
    p1 = texture(tex_map, t1);

    frag = vec4(p1.xyz * p1.w + p0.xyz * (1.0 - p1.w), 1.0);
}
