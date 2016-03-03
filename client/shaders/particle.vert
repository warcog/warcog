#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 p;
layout (location = 1) in uint d;

layout (location = 0) out vec3 position;
layout (location = 1) out uint data;

void main()
{
    position = p;
    data = d;
}
