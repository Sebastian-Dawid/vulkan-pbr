#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec2 tex_coord;

void main()
{
    gl_Position = vec4(pos, 1.0);
}
