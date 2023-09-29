#version 450

layout (binding = 0) uniform ubo_t
{
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 tex_coord;

layout (location = 0) out vec3 frag_color;
layout (location = 1) out vec2 frag_tex_coord;

void main()
{
    gl_Position = ubo.projection * ubo.view * ubo. model * vec4(pos, 1.0);
    frag_color = color;
    frag_tex_coord = tex_coord;
}
