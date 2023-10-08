#version 450

layout (location = 0) in vec2 frag_tex_coord;

layout (binding = 0) uniform sampler2D g_pos;
layout (binding = 1) uniform sampler2D g_normal;
layout (binding = 2) uniform sampler2D g_albedo;

layout (binding = 3) uniform light_t
{
    vec3 pos;
    vec3 color;
    vec3 view_pos;

    float linear;
    float quadratic;
} light;

layout (location = 0) out vec4 out_color;


void main()
{
    vec3 frag_pos = texture(g_pos, frag_tex_coord).rgb;
    vec3 normal   = texture(g_normal, frag_tex_coord).rgb;
    vec3 diffuse  = texture(g_albedo, frag_tex_coord).rgb;

    vec3 lighting = diffuse * 0.1; // ambient
    vec3 view_dir = normalize(light.view_pos - frag_pos);

    // diffuse
    vec3 light_dir = normalize(light.pos - frag_pos);
    vec3 diff = max(dot(normal, light_dir), 0.0) * diffuse * light.color;

    // specular
    vec3 half_way_dir = normalize(light_dir + view_dir);
    float spec = pow(max(dot(normal, half_way_dir), 0.0), 16.0);
    vec3 specular = light.color * spec;

    float distance = length(light.pos - frag_pos);
    float attenuation = 1.0 / (1.0 + light.linear * distance + light.quadratic * distance * distance);

    diff *= attenuation;
    specular *= attenuation;
    lighting += diff + specular;

    out_color = vec4(lighting, 1.0);
}
