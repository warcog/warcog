#version 330
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;
uniform vec2 matrix;
uniform float var0;
in vec4 position[];
in uvec2 data[];
out vec2 coord;
flat out vec4 color;
flat out uint textured;

void main()
{
    vec2 a, b, c;
    vec4 pos, tex;

    a = matrix * position[0].xy + vec2(-1.0, 1.0);
    b = position[0].zw;
    c = vec2(float(data[0].x & 0xFFFu), float((data[0].x >> 12u) & 0xFFFu));
    tex = vec4(c, c + b) * (1.0 / 1024.0);

    if ((data[0].x & (1u << 24u)) != 0u)
        b *= var0;
    if ((data[0].x & (1u << 25u)) != 0u)
        b *= 0.5;
    if ((data[0].x & (1u << 26u)) != 0u)
        b *= 0.9375;
    pos = vec4(a, a + matrix * b);

    color = vec4(float(data[0].y & 0xFFu), float((data[0].y >> 8u) & 0xFFu),
                 float((data[0].y >> 16u) & 0xFFu), float(data[0].y >> 24u)) * (1.0 / 255.0);

    textured = (data[0].x >> 30u);

    coord = tex.xy;
    gl_Position = vec4(pos.xy, 0.0, 1.0);
    EmitVertex();

    coord = tex.xw;
    gl_Position = vec4(pos.xw, 0.0, 1.0);
    EmitVertex();

    coord = tex.zy;
    gl_Position = vec4(pos.zy, 0.0, 1.0);
    EmitVertex();

    coord = tex.zw;
    gl_Position = vec4(pos.zw, 0.0, 1.0);
    if ((data[0].x & (1u << 27u)) == 0u)
        EmitVertex();

    EndPrimitive();
}
