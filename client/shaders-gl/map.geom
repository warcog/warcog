#version 330
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;
uniform mat4 matrix;
in uint data[];
in vec4 height[];
out vec4 tex;
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
    gl_Position = matrix * vec4(p.xy, height[0].x, 1.0);
    EmitVertex();

    tex = vec4(s.zy, t.zy);
    gl_Position = matrix * vec4(p.zy, height[0].y, 1.0);
    EmitVertex();

    tex = vec4(s.xw, t.xw);
    gl_Position = matrix * vec4(p.xw, height[0].z, 1.0);
    EmitVertex();

    tex = vec4(s.zw, t.zw);
    gl_Position = matrix * vec4(p.zw, height[0].w, 1.0);
    EmitVertex();

    EndPrimitive();
}
