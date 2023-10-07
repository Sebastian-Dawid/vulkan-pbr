#version 450

layout (location = 0) out vec3 g_position;
layout (location = 1) out vec3 g_normal;
layout (location = 2) out vec4 g_albedo;

layout (location = 0) in vec3 frag_pos;
layout (location = 1) in vec3 frag_normal;
layout (location = 2) in vec2 frag_tex_coord;

layout (binding = 1) uniform sampler2D texture_sampler;

void main()
{
    g_position = frag_pos;
    g_albedo = vec4(texture(texture_sampler, frag_tex_coord).rgb, 1.0);
    g_normal = normalize(frag_normal);
}
