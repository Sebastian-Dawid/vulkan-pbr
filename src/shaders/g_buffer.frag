#version 450

layout (location = 0) out vec3 g_position;
layout (location = 1) out vec3 g_normal;
layout (location = 2) out vec4 g_albedo;
layout (location = 3) out vec3 g_pbr;

layout (location = 0) in vec3 frag_pos;
layout (location = 1) in vec3 frag_normal;
layout (location = 2) in vec2 frag_tex_coord;
layout (location = 3) in mat3 frag_TBN;

layout (binding = 1) uniform sampler2D texture_albedo;
layout (binding = 2) uniform sampler2D texture_specular;
layout (binding = 3) uniform sampler2D texture_normal;
layout (binding = 4) uniform sampler2D texture_metallic;
layout (binding = 5) uniform sampler2D texture_roughness;
layout (binding = 6) uniform sampler2D texture_ao;

layout (binding = 7) uniform flags_t
{
    bool normal_map;
} flags;

void main()
{
    g_position = frag_pos;
    g_albedo.rgb = texture(texture_albedo, frag_tex_coord).rgb;
    g_albedo.a = texture(texture_specular, frag_tex_coord).r;
    if (flags.normal_map)
    {
        g_normal = normalize(frag_TBN * (texture(texture_normal, frag_tex_coord).rgb * 2.0 - 1.0));
    }
    else
    {
        g_normal = normalize(frag_normal);
    }
    g_pbr = vec3(texture(texture_metallic, frag_tex_coord).r, texture(texture_roughness, frag_tex_coord).r, texture(texture_ao, frag_tex_coord).r);
}
