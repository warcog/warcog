#version 330
uniform sampler2D zero;
in vec2 tex;
flat in vec4 col;
layout (location = 0) out vec4 frag;
void main()
{
    frag = col * texture2D(zero, tex);
}
