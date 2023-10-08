#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>

struct recording_settings_t
{
    VkRenderPassBeginInfo render_pass_info{};
    VkPipelineBindPoint bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
    std::function<void(VkCommandBuffer)> draw_command;

    void populate_defaults(VkRenderPass render_pass, VkFramebuffer framebuffer, VkExtent2D extent, const std::vector<VkClearValue>& clear_colors);
};

class command_buffers_t
{
    public:
        std::vector<VkCommandBuffer> command_buffers;
        std::int32_t init(const VkCommandPool& command_pool, const VkDevice* device, const std::uint32_t nr_buffers);
        std::int32_t record(std::uint32_t buffer, const recording_settings_t& settings);
        command_buffers_t();
        ~command_buffers_t();
};

VkCommandBuffer begin_single_time_commands(VkDevice device, VkCommandPool pool);
void end_single_time_commands(VkCommandPool pool, VkCommandBuffer command_buffer, VkDevice device, VkQueue queue);
