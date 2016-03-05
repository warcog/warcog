#version 330
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;
uniform mat4 matrix;
uniform float var0;
uniform uvec4 var1[256];
in vec3 pos[];
in uint data[];
out vec2 tex;
flat out vec4 col;
void main()
{
    vec4 p, q;
    vec2 s, t;
    uvec4 info;
    uint frame;

    info = var1[data[0] & 0xFFu];

    p = matrix * vec4(pos[0], 1.0);
    s = vec2(2.0 * var0, 2.0);

    frame = ((info.x >> 4u) & 0x3Fu);
    frame += (((info.x >> 10u) & 0x3Fu) * (data[0] >> 16u) + (info.x >> 17u)) / (info.x >> 16u);

    t = vec2(float(((info.x << 3u) & 24u) | (frame & 7u)),
             float(((info.x << 1u) & 24u) | (frame >> 3u)));

    q = vec4(t, t + vec2(1.0, 1.0)) * (1.0 / 32.0);

    col = vec4(float(info.y & 0xFFu), float((info.y >> 8u) & 0xFFu),
               float((info.y >> 16u) & 0xFFu), float(info.y >> 24u)) * (1.0 / 255.0);

    tex = q.xy;
    gl_Position = p + vec4(-s.x, -s.y, 0.0, 0.0);
    EmitVertex();

    tex = q.zy;
    gl_Position = p + vec4(s.x, -s.y, 0.0, 0.0);
    EmitVertex();

    tex = q.xw;
    gl_Position = p + vec4(-s.x, s.y, 0.0, 0.0);
    EmitVertex();

    tex = q.zw;
    gl_Position = p + vec4(s.x, s.y, 0.0, 0.0);
    EmitVertex();

    EndPrimitive();
}
