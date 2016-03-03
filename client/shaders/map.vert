#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in uint d;
layout (location = 1) in vec4 h;

layout (location = 0) out uint data;
layout (location = 1) out vec4 height;

void main()
{
    data = d;
    height = h;
}
