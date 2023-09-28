#include "vulkan_vertex.h"

VkVertexInputBindingDescription vertex_t::get_binding_description()
{
    VkVertexInputBindingDescription binding_description{};
    binding_description.binding = 0;
    binding_description.stride = sizeof(vertex_t);
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return binding_description;
}

std::array<VkVertexInputAttributeDescription, 3> vertex_t::get_attribute_description()
{
    std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions;
    attribute_descriptions[0].binding = 0;
    attribute_descriptions[0].location = 0;
    attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_descriptions[0].offset = offsetof(vertex_t, pos);
    
    attribute_descriptions[1].binding = 0;
    attribute_descriptions[1].location = 1;
    attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[1].offset = offsetof(vertex_t, color);
    
    attribute_descriptions[2].binding = 0;
    attribute_descriptions[2].location = 2;
    attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_descriptions[2].offset = offsetof(vertex_t, tex_coord);
    
    return attribute_descriptions;
}
