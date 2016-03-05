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

layout (location = 0) out vec4 tex;

void main()
{
    float x, y;
    vec2 u, v;
    vec4 s, t, p;

    x = float((data[0] & 0xFFu) << 4u);
    y = float((data[0] >> 4u) & 0xFF0u);
    p = vec4(x, y, x + 16.0, y + 16.0);

    u = vec2(float((data[0] >> 16u) & 15u), float((data[0] >> 20u) & 15u)) / 16.0;
    v = vec2(float((data[0] >> 24u) & 15u), float((data[0] >> 28u))) / 16.0;
    s = vec4(u, u + vec2(0.0625, 0.0625));
    t = vec4(v, v + vec2(0.0625, 0.0625));

    tex = vec4(s.xy, t.xy);
    gl_Position = ubuf.MVP * vec4(p.xy, height[0].x, 1.0);
    gl_Position.y = -gl_Position.y;
    gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
    EmitVertex();

    tex = vec4(s.zy, t.zy);
    gl_Position = ubuf.MVP * vec4(p.zy, height[0].y, 1.0);
    gl_Position.y = -gl_Position.y;
    gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
    EmitVertex();

    tex = vec4(s.xw, t.xw);
    gl_Position = ubuf.MVP * vec4(p.xw, height[0].z, 1.0);
    gl_Position.y = -gl_Position.y;
    gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
    EmitVertex();

    tex = vec4(s.zw, t.zw);
    gl_Position = ubuf.MVP * vec4(p.zw, height[0].w, 1.0);
    gl_Position.y = -gl_Position.y;
    gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
    EmitVertex();

    EndPrimitive();
}
