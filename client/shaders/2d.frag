#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D tex_text;
layout (binding = 2) uniform sampler2D tex_icons;
layout (binding = 8) uniform sampler2D tex_minimap;

layout (location = 0) in vec2 coord;
layout (location = 1) flat in vec4 color;
layout (location = 2) flat in float textured;

layout(location = 0) out vec4 frag;

void main()
{
    vec4 tmp;
    vec2 c;

    c = coord;
    //c.y = 1.0 - c.y;

    tmp = vec4(1.0, 1.0, 1.0, 1.0);
    if (textured == 1.0) {
        tmp *= texture(tex_text, c);
    } else if (textured == 2.0) {
        tmp *= texture(tex_icons, c);
    } else if (textured == 3.0) {
        tmp *= texture(tex_minimap, c);
    }

    frag = tmp * color;
}
