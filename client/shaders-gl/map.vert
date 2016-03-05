#version 330
layout (location = 0) in uint d;
layout (location = 1) in vec4 h;
out uint data;
out vec4 height;

void main()
{
    data = d;
    height = h;
}
