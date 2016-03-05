#version 330
uniform sampler2D var0, var1;
uniform vec2 matrix;
layout (location = 0) out vec4 frag;

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
