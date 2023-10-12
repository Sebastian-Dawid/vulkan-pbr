#version 450

layout (binding = 0) uniform ubo_t
{
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec2 tex_coord;

layout (location = 0) out vec3 frag_pos;
layout (location = 1) out vec3 frag_normal;
layout (location = 2) out vec2 frag_tex_coord;
layout (location = 3) out mat3 frag_TBN;

void main()
{
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(pos, 1.0);
    frag_pos = vec3(ubo.model * vec4(pos, 1.0));
    frag_tex_coord = tex_coord;
    frag_normal = tangent;//mat3(transpose(inverse(ubo.model))) * normal;

    mat3 normal_matrix = transpose(inverse(mat3(ubo.model)));

    vec3 T = normalize(normal_matrix * tangent);
    vec3 N = normalize(normal_matrix * normal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    frag_TBN = mat3(T, B, N);
}
