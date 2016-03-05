#version 330
layout (location = 0) in vec4 p;
layout (location = 1) in uvec2 d;
out vec4 position;
out uvec2 data;

void main()
{
    position = p;
    data = d;
}
