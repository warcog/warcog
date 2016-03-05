#version 330
uniform sampler2DArray zero;
uniform vec4 var0, var1;
uniform float layer;
uniform vec3 outline;
in vec2 x;
layout(location = 0) out vec4 frag;
layout(location = 1) out vec4 frag2;

void main()
{
    vec4 t,c;

    t = texture(zero, vec3(x, 0.0));
    c = vec4((1.0-var1.a)*t.rgb+var1.a*((1.0-t.a)*var1.rgb+t.a*t.rgb),(1.0-var1.a)*t.a+var1.a)*var0;
    if (c.a < 0.5)
        discard;

    frag = c;
    frag2 = vec4(outline, 1.0);
}
