#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

layout(std140, binding = 0) uniform buf {
    mat4 MVP;
    vec2 matrix;
    float var0;
} ubuf;

out gl_PerVertex {
    vec4 gl_Position;
};

layout (location = 0) in vec4 position[];
layout (location = 1) in uvec4 data[];

layout (location = 0) out vec2 coord;
layout (location = 1) flat out vec4 color;
layout (location = 2) flat out float textured;

void main()
{
    vec2 a, b, c;
    vec4 pos, tex;

    pos = position[0];
    /*vec4(float(data[0].x & 0xFFFFu), float(data[0].x >> 16u),
               float(data[0].y & 0xFFFFu), float(data[0].y >> 16u));*/

    a = ubuf.matrix * pos.xy + vec2(-1.0, -1.0);
    b = pos.zw;
    c = vec2(float(data[0].z & 0xFFFu), float((data[0].z >> 12u) & 0xFFFu));
    //c = vec2(float(data[0].z & 0xFFFu), float((512u >> 12u) & 0xFFFu));
    tex = vec4(c, c + b) * (1.0 / 1024.0);
    //tex = vec4(0.0, 0.0, 1.0, 1.0);

    if ((data[0].z & (1u << 24u)) != 0u)
        b *= ubuf.var0;
    if ((data[0].z & (1u << 25u)) != 0u)
        b *= 0.5;
    if ((data[0].z & (1u << 26u)) != 0u)
        b *= 0.9375;
    pos = vec4(a, a + ubuf.matrix * b);

    color = vec4(float(data[0].w & 0xFFu), float((data[0].w >> 8u) & 0xFFu),
                 float((data[0].w >> 16u) & 0xFFu), float(data[0].w >> 24u)) * (1.0 / 255.0);

    textured = float(data[0].z >> 30u);

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
    if ((data[0].z & (1u << 27u)) == 0u)
        EmitVertex();

    EndPrimitive();
}
