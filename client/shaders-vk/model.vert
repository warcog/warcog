#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(std140, binding = 0) uniform buf {
    mat4 MVP;
    vec2 matrix;
    float var0,  texture;

    vec4 team_color;
    vec4 color;

    mat4 m0;
    vec4 r0[126];
    vec4 d0[126];
} ubuf;

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;
layout (location = 2) in vec4 n;
layout (location = 3) in uvec4 g;

layout (location = 0) out vec2 x;

vec3 qrot(vec4 q, vec3 v)
{
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

void main()
{
    float w;
    vec3 v;
    int ng;

    x = tex / 32768.0 - 0.5;
    w = 1.0 / n.w;
    v = (qrot(ubuf.r0[g.x], pos * ubuf.d0[g.x].w) + ubuf.d0[g.x].xyz) * w;

    ng = int(n.w);
    if (ng > 1) {
        v += (qrot(ubuf.r0[g.y], pos * ubuf.d0[g.y].w) + ubuf.d0[g.y].xyz) *  w;
        if(ng > 2){
            v += (qrot(ubuf.r0[g.z], pos * ubuf.d0[g.z].w) + ubuf.d0[g.z].xyz) * w;
            if (ng == 4) {
                v += (qrot(ubuf.r0[g.w], pos * ubuf.d0[g.w].w) + ubuf.d0[g.w].xyz) * w;
            }
        }
    }

    //v = pos;

    gl_Position = ubuf.m0 * vec4(v, 1.0);
    // GL->VK conventions
    gl_Position.y = -gl_Position.y;
    gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
}
