#version 330
uniform mat4 matrix;
uniform vec4 r[126];
uniform vec4 d[126];

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;
layout (location = 2) in vec4 n;
layout (location = 3) in uvec4 g;

out vec2 x;

vec3 qrot(vec4 q, vec3 v)
{
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

void main()
{
    float w;
    vec3 v;
    int ng;

    x = tex / 32768.0 - 0.5;
    w = 1.0 / n.w;
    v = (qrot(r[g.x], pos * d[g.x].w) + d[g.x].xyz) * w;

    ng = int(n.w);
    if (ng > 1) {
        v += (qrot(r[g.y], pos * d[g.y].w) + d[g.y].xyz) *  w;
        if(ng > 2){
            v += (qrot(r[g.z], pos * d[g.z].w) + d[g.z].xyz) * w;
            if (ng == 4) {
                v += (qrot(r[g.w], pos * d[g.w].w) + d[g.w].xyz) * w;
            }
        }
    }
    gl_Position = matrix * vec4(v, 1.0);
}
