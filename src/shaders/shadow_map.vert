#version 450

layout (location = 0) in vec3 pos;

layout (location = 0) out vec3 frag_pos;
layout (location = 1) out vec3 frag_light_pos;

layout (binding = 0) uniform ubo_t
{
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 pos;
} ubo;

layout (push_constant) uniform push_const_t
{
    mat4 view;
} push_const;

void main()
{
    gl_Position = ubo.projection * push_const.view * ubo.model * vec4(pos, 1.0);
    frag_pos = (ubo.model * vec4(pos, 1)).xyz;
    frag_light_pos = ubo.pos;
}
