#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 4) uniform sampler2DArray zero;

//uniform vec4 var0, var1;
//uniform vec3 outline;

layout(std140, binding = 0) uniform buf {
    mat4 MVP;
    vec2 matrix;
    float var0, texture;

    vec4 team_color;
    vec4 color;
} ubuf;

layout (location = 0) in vec2 x;
layout (location = 0) out vec4 frag;

void main()
{
    vec4 t, c, tc;

    t = texture(zero, vec3(x, ubuf.texture));
    if (ubuf.texture == 16383.0)
        t = vec4(1.0, 1.0, 1.0, 1.0);

    tc = ubuf.team_color;
    c = vec4((1.0-tc.a)*t.rgb+tc.a*((1.0-t.a)*tc.rgb+t.a*t.rgb),(1.0-tc.a)*t.a+tc.a)*ubuf.color;
    if (c.a < 0.5)
        discard;

    frag = c;
    //frag2 = vec4(outline, 1.0);
}
