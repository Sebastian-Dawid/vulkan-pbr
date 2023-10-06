#version 450

layout (location = 0) in vec3 frag_pos;
layout (location = 1) in vec2 frag_tex_coord;
layout (location = 2) in vec3 frag_normal;

layout (binding = 1) uniform sampler2D tex_sampler;
layout (binding = 2) uniform light_t
{
    vec3 light_pos;
    vec3 light_color;
    vec3 view_pos;
} phong;

layout (location = 0) out vec4 out_color;


void main()
{
    vec3 ambient = phong.light_color * 0.1;
    vec3 norm = normalize(frag_normal);
    vec3 light_dir = normalize(phong.light_pos - frag_pos);
    float diff = max(dot(norm, light_dir), 0.0);
    vec3 diffuse = diff * phong.light_color;
    float spec_strength = 0.5;
    vec3 view_dir = normalize(phong.view_pos - frag_pos);
    vec3 reflect_dir = reflect(-light_dir, norm);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
    vec3 specular = spec_strength * spec * phong.light_color;
    out_color = vec4((ambient + diffuse + specular) * texture(tex_sampler, frag_tex_coord).rgb, 1.0);
}
