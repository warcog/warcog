#version 330
uniform sampler2D zero;
in vec2 tex;
flat in vec2 r;
layout(location=0) out vec4 frag;
void main()
{
    frag = vec4(vec3(0.0, 1.0, 0.0), 1.0 - abs(dot(tex, tex) - r.x) * r.y);
}
