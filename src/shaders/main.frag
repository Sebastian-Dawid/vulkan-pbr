#version 450

layout (location = 0) in vec2 frag_tex_coord;

layout (input_attachment_index = 0, binding = 0) uniform subpassInput g_pos;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput g_normal;
layout (input_attachment_index = 2, binding = 2) uniform subpassInput g_albedo;

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
    vec3 frag_pos = subpassLoad(g_pos).rgb;
    vec3 normal   = subpassLoad(g_normal).rgb;
    vec3 diffuse  = subpassLoad(g_albedo).rgb;

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
