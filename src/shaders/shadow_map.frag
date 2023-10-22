#version 450
layout (location = 0) out float frag_color;

layout (location = 0) in vec3 frag_pos;
layout (location = 1) in vec3 frag_light_pos;

void main()
{
    vec3 lv = frag_pos - frag_light_pos;
    frag_color = length(lv);
}
