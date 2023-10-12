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
    vec3 view_pos;

    float linear;
    float quadratic;
} light;

layout (location = 0) out vec4 out_color;

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

    Lo += diffuse * light.color * pbr.b;

    out_color = vec4(Lo, 1.0);
}

vec3 calc_point_light(vec3 pos, vec3 normal, vec3 diffuse, float metallic, float roughness, vec3 F0)
{
    return vec3(0.0);
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

float dir_shadow_calc(dir_light_t light, vec3 pos, vec3 normal, vec3 light_dir)
{
    float shadow = 0.0;
    mat4 i_view = ssao_active ? inverse(view) : mat4(1.0);
    vec4 light_pos = light.light_space_matrix * i_view * vec4(pos, 1.0);
    float bias = max(0.05 * (1.0 - dot(normal, light_dir)), 0.005);

    vec3 proj_coord = light_pos.xyz / light_pos.w;
    proj_coord = proj_coord * 0.5 + 0.5;
    float current_depth = proj_coord.z;

    vec2 texel_size = 1.0 / textureSize(light.shadow_map, 0);
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcf_depth = texture(light.shadow_map, proj_coord.xy + vec2(x, y) * texel_size).r;
            shadow += current_depth - bias  > pcf_depth ? 1.0 : 0.0;
        }
    }
    shadow /= 9;

    if (proj_coord.z > 1.0)
    {
        shadow = 0.0;
    }

    return shadow;
}
