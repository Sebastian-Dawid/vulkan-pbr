#include "vulkan_base/vulkan_base.h"
#include "vulkan_base/vulkan_vertex.h"
#include "model_base/model_base.h"
#include <chrono>
#include <cstring>

struct ubo_t
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
};

struct blinn_phong_t
{
    glm::vec3 light_pos;
    glm::vec3 light_color;
    glm::vec3 view_pos;
};

int main()
{

    vulkan_context_t vk_context("Vulkan Template");
    if (!vk_context.initialized)
    {
        glfwTerminate();
        return -1;
    }

    model_t model("./models/backpack/backpack.obj");
    if (!model.is_initialized()) return -1;
    auto bufs = model.set_up_buffer(&vk_context);
    if (!bufs.has_value()) return -1;
    buffer_t* vertex_buffer = std::get<0>(bufs.value());
    buffer_t* index_buffer = std::get<1>(bufs.value());

    std::vector<std::tuple<std::uint32_t, VkDeviceSize, void*, VkDescriptorType, bool>> descriptor_config;
    std::vector<buffer_t*> ubo_buffers;
    for (std::uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        buffer_settings_t ubo_settings;
        ubo_settings.size = sizeof(ubo_t);
        ubo_settings.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        ubo_settings.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if (vk_context.add_buffer(ubo_settings) != 0) return -1;
        buffer_t* ubo_buffer = vk_context.get_last_buffer();
        ubo_buffer->map_memory();
        ubo_buffers.push_back(ubo_buffer);
    }
    descriptor_config.push_back(std::make_tuple(0, sizeof(ubo_t), &ubo_buffers[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, true));

    image_settings_t image_settings;
    if (vk_context.add_image("./models/backpack/albedo.jpg", image_settings, true) != 0) return -1;
    image_t* img = vk_context.get_image(0);
    descriptor_config.push_back(std::make_tuple(1, 0, img, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, false));

    std::vector<buffer_t*> blinn_phong_buffers;
    for (std::uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        buffer_settings_t blinn_phong_settings;
        blinn_phong_settings.size = sizeof(blinn_phong_t);
        blinn_phong_settings.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        blinn_phong_settings.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        blinn_phong_settings.binding = 2;
        if (vk_context.add_buffer(blinn_phong_settings) != 0) return -1;
        buffer_t* blinn_phong_buffer = vk_context.get_last_buffer();
        blinn_phong_buffer->map_memory();
        blinn_phong_buffers.push_back(blinn_phong_buffer);
    }
    descriptor_config.push_back(std::make_tuple(2, sizeof(blinn_phong_t), &blinn_phong_buffers[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, true));

    VkDescriptorSetLayoutBinding phong_layout_binding = { 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    std::vector<VkDescriptorSetLayoutBinding> bindings = { UBO_LAYOUT_BINDING, SAMPLER_LAYOUT_BINDING, phong_layout_binding };

    
    std::vector<image_t*> g_buffer;
    image_settings_t g_buffer_settings;
    g_buffer_settings.sample_count = VK_SAMPLE_COUNT_8_BIT;
    g_buffer_settings.format = vk_context.swap_chain->format.format;
    g_buffer_settings.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    // POS
    g_buffer.push_back(new image_t(&vk_context.physical_device, &vk_context.command_pool));
    g_buffer.back()->init_color_buffer(g_buffer_settings, vk_context.get_swap_chain_extent(), vk_context.device);
    // NORMAL
    g_buffer.push_back(new image_t(&vk_context.physical_device, &vk_context.command_pool));
    g_buffer.back()->init_color_buffer(g_buffer_settings, vk_context.get_swap_chain_extent(), vk_context.device);
    // ALBEDO
    g_buffer.push_back(new image_t(&vk_context.physical_device, &vk_context.command_pool));
    g_buffer.back()->init_color_buffer(g_buffer_settings, vk_context.get_swap_chain_extent(), vk_context.device);

    render_pass_settings_t render_pass_settings;
    render_pass_settings.populate_defaults(vk_context.swap_chain->format.format, VK_SAMPLE_COUNT_8_BIT, &vk_context.physical_device, 3, 1, 0);
    render_pass_t* g_render_pass = new render_pass_t();
    g_render_pass->init(render_pass_settings, vk_context.device->device);

    VkExtent2D swap_chain_extent = vk_context.get_swap_chain_extent();
    std::vector<framebuffer_attachment_t> g_attachments = { {&g_buffer[0], IMAGE}, {&g_buffer[1], IMAGE}, {&g_buffer[2], IMAGE}, {&vk_context.depth_buffer, IMAGE} };
    g_render_pass->add_framebuffer(swap_chain_extent.width, swap_chain_extent.height, g_attachments);

    vk_context.render_passes.push_back(g_render_pass);

    vk_context.add_descriptor_set_layout(bindings);
    pipeline_shaders_t g_shaders = { "./build/target/shaders/g_buffer.vert.spv", std::nullopt, "./build/target/shaders/g_buffer.frag.spv" };
    pipeline_settings_t g_pipeline_settings;
    g_pipeline_settings.populate_defaults(vk_context.get_descriptor_set_layouts(), vk_context.render_passes[1], 3);
    g_pipeline_settings.multisampling.rasterizationSamples = vk_context.msaa_samples;
    if (vk_context.add_pipeline(g_shaders, g_pipeline_settings) != 0) return -1;
    
    vk_context.add_descriptor_set_layout(bindings);
    pipeline_shaders_t shaders = { "./build/target/shaders/main.vert.spv", std::nullopt, "./build/target/shaders/main.frag.spv" };
    pipeline_settings_t pipeline_settings;
    pipeline_settings.populate_defaults(vk_context.get_descriptor_set_layouts(), vk_context.render_passes[0]);
    pipeline_settings.multisampling.rasterizationSamples = vk_context.msaa_samples;
    if (vk_context.add_pipeline(shaders, pipeline_settings) != 0) return -1;

    std::vector<descriptor_pool_t*> pools = *vk_context.get_descriptor_pools();
    std::for_each(pools.begin(), pools.end(), [&] (descriptor_pool_t* pool) { pool->configure_descriptors(descriptor_config); });

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
        vkCmdBindIndexBuffer(command_buffer, index_buffer->buffer, 0, VK_INDEX_TYPE_UINT32);
        context->bind_descriptor_sets(command_buffer, 0, 0);

        vkCmdDrawIndexed(command_buffer, static_cast<std::uint32_t>(model.indices.size()), 1, 0, 0, 0);
    };

    vk_context.main_loop([&]
    {
        while (!glfwWindowShouldClose(vk_context.window))
        {
            glfwPollEvents();

            static std::chrono::time_point start_time = std::chrono::high_resolution_clock::now();
            std::chrono::time_point current_time = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();
            static blinn_phong_t blinn_phong = { {4.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {10.0f, 0.0f, 0.0f} };
            std::memcpy(blinn_phong_buffers[vk_context.get_current_frame()]->mapped_memory, &blinn_phong, sizeof(blinn_phong_t));
            static ubo_t ubo;
            ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            ubo.view = glm::lookAt(glm::vec3(10.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            ubo.projection = glm::perspective(glm::radians(45.0f), vk_context.get_swap_chain_extent().width / (float) vk_context.get_swap_chain_extent().height, 0.1f, 100.0f);
            ubo.projection[1][1] *= -1;
            std::memcpy(ubo_buffers[vk_context.get_current_frame()]->mapped_memory, &ubo, sizeof(ubo_t));
            
            if (vk_context.draw_frame(draw_command) != 0)
            {
                break;
            }
        }
    });

    std::for_each(g_buffer.begin(), g_buffer.end(), [] (image_t* img) { delete img; } );

    glfwTerminate();
    return 0;
}
