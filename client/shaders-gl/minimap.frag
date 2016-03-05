#version 330
uniform usampler2D zero;
uniform sampler2D var1;
uniform float var0;
layout (location = 0) out vec4 frag;
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
