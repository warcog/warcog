#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

layout(std140, binding = 0) uniform buf {
    mat4 MVP;
    vec2 matrix;
    float var0,  texture;

    vec4 team_color;
    vec4 color;

    mat4 m0;
    vec4 r0[126];
    vec4 d0[126];

    uvec4 info[256];
} ubuf;

out gl_PerVertex {
    vec4 gl_Position;
};

layout (location = 0) in vec3 pos[];
layout (location = 1) in uint data[];

layout (location = 0) out vec2 tex;
layout (location = 1) flat out vec4 col;

void main()
{
    vec4 p, q;
    vec2 s, t;
    uvec4 info;
    uint frame;

    info = ubuf.info[data[0] & 0xFFu];

    p = ubuf.MVP * vec4(pos[0], 1.0);
    s = vec2(2.0, 2.0);

    frame = ((info.x >> 4u) & 0x3Fu);
    frame += (((info.x >> 10u) & 0x3Fu) * (data[0] >> 16u) + (info.x >> 17u)) / (info.x >> 16u);

    t = vec2(float(((info.x << 3u) & 24u) | (frame & 7u)),
             float(((info.x << 1u) & 24u) | (frame >> 3u)));

    q = vec4(t, t + vec2(1.0, 1.0)) * (1.0 / 32.0);

    col = vec4(float(info.y & 0xFFu), float((info.y >> 8u) & 0xFFu),
               float((info.y >> 16u) & 0xFFu), float(info.y >> 24u)) * (1.0 / 255.0);

    tex = q.xy;
    gl_Position = p + vec4(-s.x, -s.y, 0.0, 0.0);
    gl_Position.y = -gl_Position.y;
    gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
    EmitVertex();

    tex = q.zy;
    gl_Position = p + vec4(s.x, -s.y, 0.0, 0.0);
    gl_Position.y = -gl_Position.y;
    gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
    EmitVertex();

    tex = q.xw;
    gl_Position = p + vec4(-s.x, s.y, 0.0, 0.0);
    gl_Position.y = -gl_Position.y;
    gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
    EmitVertex();

    tex = q.zw;
    gl_Position = p + vec4(s.x, s.y, 0.0, 0.0);
    gl_Position.y = -gl_Position.y;
    gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
    EmitVertex();

    EndPrimitive();
}
