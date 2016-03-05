#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

layout(std140, binding = 0) uniform buf {
    mat4 MVP;
} ubuf;

out gl_PerVertex {
    vec4 gl_Position;
};

layout (location = 0) in uint data[];
layout (location = 1) in vec4 height[];
layout (location = 2) in vec2 origin[];

layout (location = 0) out vec2 tex;
layout (location = 1) flat out vec2 r;

void main()
{
    float x, y, t;
    vec4 s, p;

    x = float((data[0] & 0xFFu) << 4u);
    y = float((data[0] >> 4u) & 0xFF0u);
    p = vec4(x, y, x + 16.0, y + 16.0);

    t = float((data[0] >> 16u) & 0xFFu);
    r = vec2(t * t, 1.0 / t);

    s = vec4(origin[0], origin[0] + vec2(16.0, 16.0));

    tex = s.xy;
    gl_Position = ubuf.MVP * vec4(p.xy, height[0].x, 1.0);
    gl_Position.y = -gl_Position.y;
    gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
    EmitVertex();

    tex = s.zy;
    gl_Position = ubuf.MVP * vec4(p.zy, height[0].y, 1.0);
    gl_Position.y = -gl_Position.y;
    gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
    EmitVertex();

    tex = s.xw;
    gl_Position = ubuf.MVP * vec4(p.xw, height[0].z, 1.0);
    gl_Position.y = -gl_Position.y;
    gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
    EmitVertex();

    tex = s.zw;
    gl_Position = ubuf.MVP * vec4(p.zw, height[0].w, 1.0);
    gl_Position.y = -gl_Position.y;
    gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
    EmitVertex();

    EndPrimitive();
}
