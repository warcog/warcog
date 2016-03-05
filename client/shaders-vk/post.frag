#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 3) uniform sampler2D tex;

layout (location = 0) in vec4 texcoord;
//layout (location = 1) flat in vec4 color;

layout (location = 0) out vec4 uFragColor;

void main()
{
   //uFragColor = texture(tex, texcoord.xy + vec2(sin(texcoord.x * 16.0) / 16.0, 0.0));
   uFragColor = vec4(texture(tex, texcoord.xy).rgb, 1.0);
   //uFragColor = texcoord;
}
