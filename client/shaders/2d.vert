#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 p;
layout (location = 1) in uvec2 d;

layout (location = 0) out vec4 position;
layout (location = 1) out uvec4 data;

void main()
{
    position = p;
    data = uvec4(0u, 0u, d);
}
