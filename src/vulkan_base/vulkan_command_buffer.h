#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>

class command_buffers_t
{
    public:
        std::vector<VkCommandBuffer> command_buffers;
        std::int32_t init(const VkCommandPool& command_pool, const VkDevice* device, const std::uint32_t nr_buffers);
        std::int32_t record(std::uint32_t buffer, std::function<void(VkCommandBuffer)> func);
        command_buffers_t();
        ~command_buffers_t();
};

VkRenderPassBeginInfo populate_render_pass_begin_info(VkRenderPass render_pass, VkFramebuffer framebuffer, VkExtent2D extent, const std::vector<VkClearValue>& clear_colors);
VkCommandBuffer begin_single_time_commands(VkDevice device, VkCommandPool pool);
void end_single_time_commands(VkCommandPool pool, VkCommandBuffer command_buffer, VkDevice device, VkQueue queue);
