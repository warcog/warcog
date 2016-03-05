#version 330
layout (location = 0) in vec3 p;
layout (location = 1) in uint d;
out vec3 pos;
out uint data;

void main()
{
    pos = p;
    data = d;
}
