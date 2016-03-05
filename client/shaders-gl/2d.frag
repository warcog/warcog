#version 330
uniform sampler2D zero, var1, var2;
in vec2 coord;
flat in vec4 color;
flat in uint textured;
layout(location = 0) out vec4 frag;

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
