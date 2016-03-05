#version 330
layout (location = 0) in uint d;
layout (location = 1) in vec4 h;
layout (location = 2) in vec2 o;
out uint data;
out vec4 height;
out vec2 origin;

void main()
{
    data = d;
    height = h;
    origin = o;
}
