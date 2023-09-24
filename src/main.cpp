#include "vulkan_base/vulkan_base.h"

int main()
{
    vulkan_context_t vk_context("Vulkan Template");
    if (!vk_context.initialized)
    {
        glfwTerminate();
        return -1;
    }

    std::function<void(VkCommandBuffer, vulkan_context_t*)> draw_command = [] (VkCommandBuffer command_buffer, vulkan_context_t* context)
    {
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(context->get_swap_chain_extent().width);
        viewport.height = static_cast<float>(context->get_swap_chain_extent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = context->get_swap_chain_extent();
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);
        vkCmdDraw(command_buffer, 3, 1, 0, 0);
    };

    vk_context.main_loop([&]
    {
        while (!glfwWindowShouldClose(vk_context.window))
        {
            glfwPollEvents();
            if (vk_context.draw_frame(draw_command) != 0)
            {
                break;
            }
        }
    });

    glfwTerminate();
    return 0;
}
