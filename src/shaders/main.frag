#version 450

layout (location = 0) in vec2 frag_tex_coord;

layout (input_attachment_index = 0, binding = 0) uniform subpassInput g_pos;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput g_normal;
layout (input_attachment_index = 2, binding = 2) uniform subpassInput g_albedo;
layout (input_attachment_index = 3, binding = 3) uniform subpassInput g_pbr;

layout (binding = 4) uniform light_t
{
    vec3 pos;
    vec3 color;
    vec3 ambient;
    vec3 view_pos;

    float linear;
    float quadratic;
} light;

layout (binding = 5) uniform view_t
{
    vec3 pos;
    mat4 mat;
} view;

layout (location = 0) out vec4 out_color;

const float PI = 3.14159265359;

vec3 calc_point_light(vec3 pos, vec3 normal, vec3 diffuse, float metallic, float roughness, vec3 F0);

float distribution_ggx(vec3 n, vec3 h, float roughness);
float geometry_schlick_ggx(float nv, float roughness);
float geometry_smith(vec3 n, vec3 v, vec3 l, float roughness);
vec3 fresnel_schlick(float cos_theta, vec3 f0);

void main()
{
    vec3 frag_pos = subpassLoad(g_pos).rgb;
    vec3 normal   = subpassLoad(g_normal).rgb;
    vec3 diffuse  = subpassLoad(g_albedo).rgb;
    vec3 pbr      = subpassLoad(g_pbr).rgb;
    
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, diffuse, pbr.r);
    vec3 Lo = vec3(0.0);

    Lo += calc_point_light(frag_pos, normal, diffuse, pbr.r, pbr.g, F0);

    Lo += diffuse * light.ambient * pbr.b;

    out_color = vec4(Lo, 1.0);
}

vec3 calc_point_light(vec3 pos, vec3 normal, vec3 albedo, float metallic, float roughness, vec3 F0)
{
    vec3 v = normalize(view.pos - pos);
    vec3 l = normalize(light.pos - pos);
    vec3 h = normalize(l + v);
    
    float dist = length(light.pos - pos);
    float attenuation = 1.0 / (dist * dist);

    vec3 radiance = light.color * attenuation;

    float NDF = distribution_ggx(normal, h, roughness);
    float G = geometry_smith(normal, v, l, roughness);
    vec3 F = fresnel_schlick(max(dot(h, v), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, v), 0.0) * max(dot(normal, l), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float nl = max(dot(normal, l), 0.0);

    return ((kD * albedo / PI + specular) * radiance * nl);
}

float distribution_ggx(vec3 n, vec3 h, float roughness)
{
    float a   = roughness * roughness;
    float a2  = a * a;
    float nh  = max(dot(n, h), 0.0);
    float nh2 = nh * nh;

    float nom   = a2;
    float denom = (nh2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float geometry_schlick_ggx(float nv, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    float nom = nv;
    float denom = nv * (1.0 - k) + k;

    return nom / denom;
}

float geometry_smith(vec3 n, vec3 v, vec3 l, float roughness)
{
    float nv = max(dot(n, v), 0.0);
    float nl = max(dot(n, l), 0.0);
    float ggx2 = geometry_schlick_ggx(nv, roughness);
    float ggx1 = geometry_schlick_ggx(nl, roughness);

    return ggx2 * ggx1;
}

vec3 fresnel_schlick(float cos_theta, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}
