#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 5) uniform sampler2D tex;

layout (location = 0) in vec2 coord;
layout (location = 1) flat in vec4 color;

layout(location = 0) out vec4 frag;

void main()
{
    frag = color * texture(tex, coord);
}
