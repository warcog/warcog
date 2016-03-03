#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 tex;
layout (location = 1) flat in vec2 r;

layout (location = 0) out vec4 frag;

void main()
{
    frag = vec4(vec3(0.0, 1.0, 0.0), 1.0 - abs(dot(tex, tex) - r.x) * r.y);
}
