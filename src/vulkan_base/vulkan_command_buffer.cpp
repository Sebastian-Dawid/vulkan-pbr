#include "vulkan_command_buffer.h"
#include "vulkan_constants.h"

#include <iostream>

void recording_settings_t::populate_defaults(VkRenderPass render_pass, VkFramebuffer framebuffer, VkExtent2D extent, VkPipeline pipeline,
        const std::vector<VkClearValue>& clear_colors)
{
    this->render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    this->render_pass_info.renderPass = render_pass;
    this->render_pass_info.framebuffer = framebuffer;
    this->render_pass_info.renderArea.extent = extent;
    this->render_pass_info.renderArea.offset = { 0, 0 };
    this->render_pass_info.clearValueCount = static_cast<std::uint32_t>(clear_colors.size());
    this->render_pass_info.pClearValues = clear_colors.data();
    this->pipeline = pipeline;
}

std::int32_t command_buffers_t::init(const VkCommandPool& command_pool, const VkDevice* device, const std::uint32_t nr_buffers)
{
    this->command_buffers.resize(MAX_FRAMES_IN_FLIGHT * nr_buffers);

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = static_cast<std::uint32_t>(this->command_buffers.size());

    if (vkAllocateCommandBuffers(*device, &alloc_info, this->command_buffers.data()) != VK_SUCCESS)
    {
        std::cerr << "Failed to allocate command buffers!" << std::endl;
        return -1;
    }

    return 0;
}

std::int32_t command_buffers_t::record(std::uint32_t buffer, const recording_settings_t& settings)
{
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(this->command_buffers[buffer], &begin_info) != VK_SUCCESS)
    {
        std::cerr << "Failed to begin recording command buffer!" << std::endl;
        return -1;
    }

    vkCmdBeginRenderPass(this->command_buffers[buffer], &settings.render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(this->command_buffers[buffer], settings.bind_point, settings.pipeline);
    settings.draw_command(this->command_buffers[buffer]);
    vkCmdEndRenderPass(this->command_buffers[buffer]);

    if (vkEndCommandBuffer(this->command_buffers[buffer]) != VK_SUCCESS)
    {
        std::cerr << "Failed to record command buffer!" << std::endl;
        return -1;
    }

    return 0;
}

command_buffers_t::command_buffers_t()
{}

command_buffers_t::~command_buffers_t()
{}

VkCommandBuffer begin_single_time_commands(VkDevice device, VkCommandPool pool)
{
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = pool;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer buffer;
    vkAllocateCommandBuffers(device, &alloc_info, &buffer);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(buffer, &begin_info);

    return buffer;
}

void end_single_time_commands(VkCommandPool pool, VkCommandBuffer command_buffer, VkDevice device, VkQueue queue)
{
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, pool, 1, &command_buffer);
}
