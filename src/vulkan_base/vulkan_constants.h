#pragma once

#include <vector>
#include <cstdint>
#include <vulkan/vulkan_core.h>

inline const std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_AMD_MIXED_ATTACHMENT_SAMPLES_EXTENSION_NAME };
inline const std::uint32_t MAX_FRAMES_IN_FLIGHT = 2;
inline const VkDescriptorSetLayoutBinding UBO_LAYOUT_BINDING = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr };
inline const VkDescriptorSetLayoutBinding SAMPLER_LAYOUT_BINDING = { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
inline const std::vector<VkClearValue> CLEAR_COLORS = {{{{0.0f, 0.0f, 0.0f, 1.0f}}}, {{{1.0f, 0}}}};
inline const std::vector<VkClearValue> G_CLEAR_COLORS = {{{{0.0f, 0.0f, 0.0f, 1.0f}}}, {{{0.0f, 0.0f, 0.0f, 1.0f}}}, {{{0.0f, 0.0f, 0.0f, 1.0f}}}, {{{1.0f, 0}}},
    {}, {}, {}, {{{0.0f, 0.0f, 0.0f, 1.0f}}}};
