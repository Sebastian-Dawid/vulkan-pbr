#include "vulkan_command_buffer.h"
#include "vulkan_constants.h"

#include <iostream>

void recording_settings_t::populate_defaults(VkRenderPass render_pass, VkFramebuffer framebuffer, VkExtent2D extent, VkPipeline pipeline)
{
    this->render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    this->render_pass_info.renderPass = render_pass;
    this->render_pass_info.framebuffer = framebuffer;
    this->render_pass_info.renderArea.extent = extent;
    this->render_pass_info.renderArea.offset = { 0, 0 };
    VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    this->render_pass_info.clearValueCount = 1;
    this->render_pass_info.pClearValues = &clear_color;
    this->pipeline = pipeline;
}

std::int32_t command_buffers_t::init(const VkCommandPool& command_pool, const VkDevice* device)
{
    this->command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

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
