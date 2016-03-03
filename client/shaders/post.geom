#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

out gl_PerVertex {
    vec4 gl_Position;
};

layout (location = 0) out vec4 texcoord;

void main(void)
{
    gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);
    texcoord = vec4(0.0, 0.0, 0.0, 0.0);
    EmitVertex();

    gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);
    texcoord = vec4(0.0, 1.0, 0.0, 0.0);
    EmitVertex();

    gl_Position = vec4(1.0, -1.0, 0.0, 1.0);
    texcoord = vec4(1.0, 0.0, 0.0, 0.0);
    EmitVertex();

    gl_Position = vec4(1.0, 1.0, 0.0, 1.0);
    texcoord = vec4(1.0, 1.0, 0.0, 0.0);
    EmitVertex();

    EndPrimitive();
}
