#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 3) in vec2 tex_coord;

layout (location = 0) out vec2 frag_tex_coord;

void main()
{
    gl_Position = vec4(pos, 1.0);
    frag_tex_coord = tex_coord;
}
