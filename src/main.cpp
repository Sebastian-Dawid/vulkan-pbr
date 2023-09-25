#include "vulkan_base/vulkan_base.h"
#include "vulkan_base/vulkan_vertex.h"

int main()
{
    vulkan_context_t vk_context("Vulkan Template");
    if (!vk_context.initialized)
    {
        glfwTerminate();
        return -1;
    }
    
    pipeline_shaders_t shaders = { "./build/target/shaders/main.vert.spv", std::nullopt, "./build/target/shaders/main.frag.spv" };
    pipeline_settings_t pipeline_settings;
    pipeline_settings.populate_defaults();
    if (vk_context.add_pipeline(shaders, pipeline_settings) != 0) return -1;
    vk_context.set_active_pipeline(0);

    std::vector<vertex_t> vertices = {
        {{ 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
        {{ 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    };

    buffer_settings_t buffer_settings;
    buffer_settings.populate_defaults(static_cast<std::uint32_t>(vertices.size()));
    if (vk_context.add_buffer(buffer_settings) != 0) return -1;
    buffer_t* vertex_buffer = vk_context.get_buffer(0);
    vertex_buffer->set_data(vertices.data());

    std::function<void(VkCommandBuffer, vulkan_context_t*)> draw_command = [&] (VkCommandBuffer command_buffer, vulkan_context_t* context)
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

        VkBuffer vertex_buffers[] = { vertex_buffer->buffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);

        vkCmdDraw(command_buffer, static_cast<std::uint32_t>(vertices.size()), 1, 0, 0);
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
