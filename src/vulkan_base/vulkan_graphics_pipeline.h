#pragma once

#include <functional>
#include <optional>
#include <cstdint>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "vulkan_render_pass.h"
#include "vulkan_logical_device.h"

struct pipeline_shaders_t
{
    std::optional<std::string> vertex;
    std::optional<std::string> geometry;
    std::optional<std::string> fragment;
};

struct pipeline_settings_t
{
    std::vector<VkVertexInputBindingDescription> vertex_binding_descriptions;
    std::vector<VkVertexInputAttributeDescription> vertex_attribute_descriptions;
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
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    const render_pass_t* render_pass = nullptr;
    std::uint32_t subpass = 0;
    std::vector<VkPushConstantRange> push_constant_ranges;

    void populate_defaults(const std::vector<VkDescriptorSetLayout>& descriptor_set_layputs, render_pass_t* render_pass, std::uint32_t color_attachment_count = 1);
};

class graphics_pipeline_t
{
    private:
        const VkDevice* device = nullptr;
        const VkAllocationCallbacks* allocator = nullptr;
        VkExtent2D swap_chain_extent;
        std::optional<VkShaderModule> create_shader_module(const std::vector<char>& code, VkDevice device);
    public:
        VkPipelineLayout pipeline_layout;
        VkPipeline pipeline;
        std::function<void(VkCommandBuffer, std::uint32_t, void*)> draw_command;

        const render_pass_t* render_pass = nullptr;
        std::int32_t init(const pipeline_shaders_t& shaders, const pipeline_settings_t& settings, const logical_device_t* device);
        graphics_pipeline_t(const render_pass_t* render_pass, VkExtent2D swap_chain_extent);
        ~graphics_pipeline_t();
};
