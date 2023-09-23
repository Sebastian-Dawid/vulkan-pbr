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

struct pipeline_settings_t
{
    VkPipelineVertexInputStateCreateInfo vertex_input{};
    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    VkPipelineViewportStateCreateInfo viewport_state{};
    std::optional<VkViewport> viewport = std::nullopt;
    std::optional<VkRect2D> scissor = std::nullopt;
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    VkPipelineMultisampleStateCreateInfo multisampling{};
    std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments;
    VkPipelineColorBlendStateCreateInfo color_blending{};
    std::vector<VkDynamicState> dynamic_states;
    VkPipelineDynamicStateCreateInfo dynamic_state{};

    void populate_defaults();
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
        std::int32_t init(const pipeline_shaders_t& shaders, const pipeline_settings_t& settings, const logical_device_t* device);
        graphics_pipeline_t(const VkRenderPass* render_pass, VkExtent2D swap_chain_extent);
        ~graphics_pipeline_t();
};
