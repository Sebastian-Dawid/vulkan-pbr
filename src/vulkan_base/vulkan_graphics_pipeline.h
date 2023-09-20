#pragma once

#include <optional>
#include <cstdint>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "vulkan_logical_device.h"

struct pipeline_shaders_t
{
    std::optional<std::string> vertex;
    std::optional<std::string> geometry;
    std::optional<std::string> fragment;
};

class graphics_pipeline_t
{
    private:
        const VkDevice* device = nullptr;
        const VkAllocationCallbacks* allocator = nullptr;
        const VkRenderPass* render_pass = nullptr;
        VkExtent2D swap_chain_extent;
        std::optional<VkShaderModule> create_shader_module(const std::vector<char>& code, VkDevice device);
    public:
        VkPipelineLayout pipeline_layout;
        VkPipeline pipeline;
        std::int32_t init(pipeline_shaders_t shaders, const logical_device_t* device);
        graphics_pipeline_t(const VkRenderPass* render_pass, VkExtent2D swap_chain_extent);
        ~graphics_pipeline_t();
};
