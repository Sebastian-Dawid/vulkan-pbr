#version 450

layout (location = 0) out vec4 frag_color;

layout (input_attachment_index = 0, binding = 0) uniform subpassInput color_buffer;

void main()
{
    vec3 color = subpassLoad(color_buffer).rgb;
    const float gamma = 2.2;
    const float exposure = 1.0;
    
    vec3 result = vec3(1.0) - exp(-color * exposure);
    //result = pow(result, vec3(1.0 / gamma));

    frag_color = vec4(result, 1.0);
}
