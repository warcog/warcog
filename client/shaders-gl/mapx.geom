#version 330
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;
uniform mat4 matrix;
in uint data[];
in vec4 height[];
in vec2 origin[];
out vec2 tex;
flat out vec2 r;
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
    gl_Position = matrix * vec4(p.xy, height[0].x, 1.0);
    EmitVertex();

    tex = s.zy;
    gl_Position = matrix * vec4(p.zy, height[0].y, 1.0);
    EmitVertex();

    tex = s.xw;
    gl_Position = matrix * vec4(p.xw, height[0].z, 1.0);
    EmitVertex();

    tex = s.zw;
    gl_Position = matrix * vec4(p.zw, height[0].w, 1.0);
    EmitVertex();

    EndPrimitive();
}
