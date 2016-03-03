/* post processing shader */
#version 330\n
void main()
{
}
~
#version 330\n
layout(points) in;
layout(triangle_strip,max_vertices=4) out;

void main()
{
    gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);
    EmitVertex();

    gl_Position = vec4(1.0, -1.0, 0.0, 1.0);
    EmitVertex();

    gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);
    EmitVertex();

    gl_Position = vec4(1.0, 1.0, 0.0, 1.0);
    EmitVertex();

    EndPrimitive();
}
~
#version 330\n
uniform sampler2D var0, var1;
uniform vec2 matrix;
layout(location=0) out vec4 frag;

void main()
{
    float m;
    vec2 coord;
    vec3 color;
    vec4 a;

    coord = gl_FragCoord.xy * matrix;
    color = texture2D(var0, coord).xyz;

    /* edge blur var1 onto var0 */
    m = texture2D(var1, coord).a;
    if (m == 0.0) {
        a  = texture2D(var1, coord + vec2(-matrix.x, -matrix.y));
        a += texture2D(var1, coord + vec2(0.0, -matrix.y));
        a += texture2D(var1, coord + vec2(matrix.x, -matrix.y));

        a += texture2D(var1, coord + vec2(-matrix.x, 0.0));
        a += texture2D(var1, coord + vec2(matrix.x, 0.0));

        a += texture2D(var1, coord + vec2(-matrix.x, matrix.y));
        a += texture2D(var1, coord + vec2(0.0, matrix.y));
        a += texture2D(var1, coord + vec2(matrix.x, matrix.y));

        a = min(a * 0.25, vec4(1.0, 1.0, 1.0, 1.0));
        color = color * (1.0 - a.a) + a.rgb * a.a;
    }

    frag = vec4(color, 1.0);
}
~
/* 2D shader */
#version 330\n
layout (location=0) in vec4 p;
layout (location=1) in uvec2 d;
out vec4 position;
out uvec2 data;

void main()
{
    position = p;
    data = d;
}
~
#version 330\n
layout(points) in;
layout(triangle_strip,max_vertices=4) out;
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
~
#version 330\n
uniform sampler2D zero, var1, var2;
in vec2 coord;
flat in vec4 color;
flat in uint textured;
layout(location=0) out vec4 frag;

void main()
{
    vec4 tmp;

    tmp = vec4(1.0, 1.0, 1.0, 1.0);
    if (textured != 0u) {
        if (textured == 1u)
            tmp *= texture2D(zero, coord);
        else if (textured == 2u)
            tmp *= texture2D(var1, coord);
        else
            tmp *= texture2D(var2, coord);
    }

    /* tmp = vec4(tmp.xyz * tmp.w, 1.0); */

    frag = tmp * color;
}
~
/* text shader */
#version 330\n
layout (location=0) in vec4 p;
layout (location=1) in uvec2 d;
out vec4 position;
out uvec2 data;

void main()
{
    position = p;
    data = d;
}
~
#version 330\n
layout(points) in;
layout(triangle_strip,max_vertices=4) out;
uniform vec2 matrix;
in vec4 position[];
in uvec2 data[];
out vec2 coord;
flat out vec4 color;
void main()
{
    vec2 a, b, c;
    vec4 pos, tex;

    a = matrix * position[0].xy + vec2(-1.0, 1.0);
    b = position[0].zw;
    c = vec2(float(data[0].x & 0xFFFu), float((data[0].x >> 12u) & 0xFFFu));
    tex = vec4(c, c + b) * (1.0 / 1024.0);
    pos = vec4(a, a + matrix * b);

    color = vec4(float(data[0].y & 0xFFu), float((data[0].y >> 8u) & 0xFFu),
                 float((data[0].y >> 16u) & 0xFFu), float(data[0].y >> 24u)) * (1.0 / 255.0);

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
    EmitVertex();

    EndPrimitive();
}
~
#version 330\n
uniform sampler2D zero;
in vec2 coord;
flat in vec4 color;
flat in uint textured;
layout(location=0) out vec4 c0;
layout(location=0,index=1) out vec4 c1;

void main()
{
    vec4 src;

    src = texture2D(zero, coord);
    c0 = src * color;
    c1 = src.a * color;
}
~
/* model */
#version 330\n
uniform mat4 matrix;
uniform vec4 r[126];
uniform vec4 d[126];

layout (location=0) in vec3 pos;
layout (location=1) in vec2 tex;
layout (location=2) in vec4 n;
layout (location=3) in uvec4 g;

out vec2 x;

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
    v = (qrot(r[g.x], pos * d[g.x].w) + d[g.x].xyz) * w;

    ng = int(n.w);
    if (ng > 1) {
        v += (qrot(r[g.y], pos * d[g.y].w) + d[g.y].xyz) *  w;
        if(ng > 2){
            v += (qrot(r[g.z], pos * d[g.z].w) + d[g.z].xyz) * w;
            if (ng == 4) {
                v += (qrot(r[g.w], pos * d[g.w].w) + d[g.w].xyz) * w;
            }
        }
    }
    gl_Position = matrix * vec4(v, 1.0);
}
~~
#version 330\n
uniform sampler2D zero;
uniform vec4 var0, var1;
uniform vec3 outline;
in vec2 x;
layout(location=0) out vec4 frag;
layout(location=1) out vec4 frag2;

void main()
{
    vec4 t,c;

    t = texture2D(zero,x);
    c = vec4((1.0-var1.a)*t.rgb+var1.a*((1.0-t.a)*var1.rgb+t.a*t.rgb),(1.0-var1.a)*t.a+var1.a)*var0;
    if (c.a < 0.5)
        discard;

    frag = c;
    frag2 = vec4(outline, 1.0);
}
~
#version 330\n
layout (location=0) in uint d;
layout (location=1) in vec4 h;
out uint data;
out vec4 height;

void main()
{
    data = d;
    height = h;
}
~
#version 330\n
layout(points) in;
layout(triangle_strip,max_vertices=4) out;
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
~
#version 330\n
uniform sampler2D zero;
in vec4 tex;
layout(location=0) out vec4 frag;
void main()
{
    vec4 bot, top;

    bot = texture2D(zero, tex.xy);
    top = texture2D(zero, tex.zw);
    frag = vec4(bot.xyz * (1.0 - top.w) + top.xyz * top.w, 1.0);
}
~
#version 330\n
layout (location=0) in uint d;
layout (location=1) in vec4 h;
layout (location=2) in vec2 o;
out uint data;
out vec4 height;
out vec2 origin;

void main()
{
    data = d;
    height = h;
    origin = o;
}
~
#version 330\n
layout(points) in;
layout(triangle_strip,max_vertices=4) out;
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
~
#version 330\n
uniform sampler2D zero;
in vec2 tex;
flat in vec2 r;
layout(location=0) out vec4 frag;
void main()
{
    frag = vec4(vec3(0.0, 1.0, 0.0), 1.0 - abs(dot(tex, tex) - r.x) * r.y);
}
~
#version 330\n
void main()
{
}
~
#version 330\n
layout(points) in;
layout(triangle_strip,max_vertices=4) out;
void main()
{
    gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);
    EmitVertex();

    gl_Position = vec4(1.0, -1.0, 0.0, 1.0);
    EmitVertex();

    gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);
    EmitVertex();

    gl_Position = vec4(1.0, 1.0, 0.0, 1.0);
    EmitVertex();

    EndPrimitive();
}
~
#version 330\n
uniform usampler2D zero;
uniform sampler2D var1;
uniform float var0;
layout(location=0) out vec4 frag;
void main()
{
    vec2 coord, t0, t1;
    vec4 p0, p1;
    uvec2 tile;

    coord = gl_FragCoord.xy * var0;
    coord = vec2(coord.x, 1.0 - coord.y);
    tile = texture(zero, coord).xy;
    t0 = vec2(float(tile.x & 15u) + 0.5, float(tile.x >> 4u) + 0.5) * (1.0 / 16.0);
    t1 = vec2(float(tile.y & 15u) + 0.5, float(tile.y >> 4u) + 0.5) * (1.0 / 16.0);
    p0 = texture2D(var1, t0);
    p1 = texture2D(var1, t1);

    frag = vec4(p1.xyz * p1.w + p0.xyz * (1.0 - p1.w), 1.0);
}
~
#version 330\n
/* particle */
layout (location=0) in vec3 p;
layout (location=1) in uint d;
out vec3 pos;
out uint data;

void main()
{
    pos = p;
    data = d;
}
~
#version 330\n
layout(points) in;
layout(triangle_strip,max_vertices=4) out;
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
~
#version 330\n
uniform sampler2D zero;
in vec2 tex;
flat in vec4 col;
layout(location=0) out vec4 frag;
void main()
{
    frag = col * texture2D(zero, tex);
}
~
