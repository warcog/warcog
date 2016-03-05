#version 330
uniform sampler2D zero;
in vec4 tex;
layout(location = 0) out vec4 frag;
void main()
{
    vec4 bot, top;

    bot = texture2D(zero, tex.xy);
    top = texture2D(zero, tex.zw);
    frag = vec4(bot.xyz * (1.0 - top.w) + top.xyz * top.w, 1.0);
}
