#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 6) uniform sampler2D zero;

layout (location = 0) in vec4 tex;

layout (location = 0) out vec4 frag;

void main()
{
    vec4 bot, top;

    bot = texture(zero, tex.xy);
    top = texture(zero, tex.zw);
    frag = vec4(bot.xyz * (1.0 - top.w) + top.xyz * top.w, 1.0);
}
