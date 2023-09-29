#include "vulkan_base/vulkan_base.h"
#include "vulkan_base/vulkan_vertex.h"
#include <chrono>
#include <cstring>

struct ubo_t
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
};

std::vector<vertex_t> vertices = {
    {{ 0.5f, -0.5f,  0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f,  0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, -0.5f,  0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    
    {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
};

std::vector<std::uint16_t> indices = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};

int main()
{

    vulkan_context_t vk_context("Vulkan Template");
    if (!vk_context.initialized)
    {
        glfwTerminate();
        return -1;
    }

    buffer_settings_t vertex_buffer_settings;
    vertex_buffer_settings.populate_defaults(static_cast<std::uint32_t>(vertices.size()));
    if (vk_context.add_buffer(vertex_buffer_settings) != 0) return -1;
    buffer_t* vertex_buffer = vk_context.get_buffer(0);
    vertex_buffer->set_staged_data(vertices.data());

    buffer_settings_t index_buffer_settings;
    index_buffer_settings.populate_defaults(static_cast<std::uint32_t>(indices.size()), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    if (vk_context.add_buffer(index_buffer_settings) != 0) return -1;
    buffer_t* index_buffer = vk_context.get_buffer(1);
    index_buffer->set_staged_data(indices.data());

    std::vector<std::tuple<std::uint32_t, VkDeviceSize, void*, VkDescriptorType, bool>> descriptor_config;
    std::vector<buffer_t*> ubo_buffers;
    for (std::uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        buffer_settings_t ubo_settings;
        ubo_settings.size = sizeof(ubo_t);
        ubo_settings.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        ubo_settings.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if (vk_context.add_buffer(ubo_settings) != 0) return -1;
        buffer_t* ubo_buffer = vk_context.get_buffer(2 + i);
        ubo_buffer->map_memory();
        ubo_buffers.push_back(ubo_buffer);
    }
    descriptor_config.push_back(std::make_tuple(0, sizeof(ubo_t), &ubo_buffers[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, true));

    image_settings_t image_settings;
    if (vk_context.add_image("./textures/texture.jpg", image_settings) != 0) return -1;
    image_t* img = vk_context.get_image(0);
    descriptor_config.push_back(std::make_tuple(1, 0, img, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, false));

    vk_context.add_descriptor_set_layout();
    pipeline_shaders_t shaders = { "./build/target/shaders/main.vert.spv", std::nullopt, "./build/target/shaders/main.frag.spv" };
    pipeline_settings_t pipeline_settings;
    pipeline_settings.populate_defaults(vk_context.get_descriptor_set_layouts());
    if (vk_context.add_pipeline(shaders, pipeline_settings) != 0) return -1;
    vk_context.set_active_pipeline(0);

    descriptor_pool_t* pool = (*vk_context.get_descriptor_pools())[0];
    pool->configure_descriptors(descriptor_config);

    std::function<void(VkCommandBuffer, vulkan_context_t*)> draw_command = [&] (VkCommandBuffer command_buffer, vulkan_context_t* context)
    {
        static std::chrono::time_point start_time = std::chrono::high_resolution_clock::now();
        std::chrono::time_point current_time = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();
        static ubo_t ubo;
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.projection = glm::perspective(glm::radians(45.0f), context->get_swap_chain_extent().width / (float) context->get_swap_chain_extent().height, 0.1f, 10.0f);
        ubo.projection[1][1] *= -1;
        std::memcpy(ubo_buffers[context->get_current_frame()]->mapped_memory, &ubo, sizeof(ubo_t));

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
        vkCmdBindIndexBuffer(command_buffer, index_buffer->buffer, 0, VK_INDEX_TYPE_UINT16);
        context->bind_descriptor_sets(command_buffer, 0, 0);

        vkCmdDrawIndexed(command_buffer, static_cast<std::uint32_t>(indices.size()), 1, 0, 0, 0);
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
