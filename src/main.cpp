#include "vulkan_base/vulkan_base.h"
#include "vulkan_base/vulkan_vertex.h"
#include "model_base/model_base.h"
#include "command_line_parser/command_line_parser.h"
#include <chrono>
#include <cstring>
#include <iostream>

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

    float linear;
    float quadratic;
};

std::vector<vertex_t> vertices = {
    { { 1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} }, // top right
    { { 1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} }, // bottom right
    { {-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} }, // bottom left
    { {-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f} }  // top left
};
std::vector<std::uint32_t> indices = { 0, 1, 2, 2, 3, 0 };

int main(int argc, char** argv)
{

    std::uint32_t width = 1280, height = 720;
    std::string model_path = "./models/backpack/backpack.obj", texture_path = "./models/backpack/albedo.jpg";
    if (argc != 1)
    {
        std::map<std::string, std::tuple<std::vector<value_type_t>, std::uint32_t>> allowed_args;
        allowed_args["--help"] = std::make_tuple(std::vector<value_type_t>{ value_type_t::NONE }, 0);
        allowed_args["--width"] = std::make_tuple(std::vector<value_type_t>{ value_type_t::UINT }, 1);
        allowed_args["--height"] = std::make_tuple(std::vector<value_type_t>{ value_type_t::UINT }, 1);
        allowed_args["--model"] = std::make_tuple(std::vector<value_type_t>{ value_type_t::STRING }, 1);
        allowed_args["--texture"] = std::make_tuple(std::vector<value_type_t>{ value_type_t::STRING }, 1);
        auto opt_res = parse_command_line_arguments(argc - 1, argv + 1, allowed_args);
        bool show_usage = false;

        if (!opt_res.has_value())
        {
            show_usage = true;
        }
        
        auto res = opt_res.value();
        if (std::find_if(res.begin(), res.end(), [](auto e){ return std::strcmp(e.first.c_str(), "--help") == 0; }) != res.end())
        {
            show_usage = true;
        }
        if (std::find_if(res.begin(), res.end(), [](auto e){ return std::strcmp(e.first.c_str(), "--width") == 0; }) != res.end())
        {
            width = res["--width"][0].u;
        }
        if (std::find_if(res.begin(), res.end(), [](auto e){ return std::strcmp(e.first.c_str(), "--height") == 0; }) != res.end())
        {
            height = res["--height"][0].u;
        }
        if (std::find_if(res.begin(), res.end(), [](auto e){ return std::strcmp(e.first.c_str(), "--model") == 0; }) != res.end())
        {
            model_path = res["--model"][0].s;
        }
        if (std::find_if(res.begin(), res.end(), [](auto e){ return std::strcmp(e.first.c_str(), "--texture") == 0; }) != res.end())
        {
            texture_path = res["--texture"][0].s;
        }

        if (show_usage)
        {
            std::cout << "usage: ./build/main [OPTIONS]" << std::endl;
            std::cout << "\tOPTIONS:" << std::endl;
            std::cout << "\t\t\"--help\":    show this message." << std::endl;
            std::cout << "\t\t\"--width\":   specify the inital width of the window." << std::endl;
            std::cout << "\t\t\"--height\":  specify the inital height of the window." << std::endl;
            std::cout << "\t\t\"--model\":   specify the path to the .obj file of the model." << std::endl;
            std::cout << "\t\t\"--texture\": specify the path to the texture of the model." << std::endl;
            return 0;
        }
    }

    vulkan_context_t vk_context("Vulkan Template", width, height);
    if (!vk_context.initialized)
    {
        glfwTerminate();
        return -1;
    }

    model_t model(model_path);
    if (!model.is_initialized()) return -1;
    auto bufs = model.set_up_buffer(&vk_context);
    if (!bufs.has_value()) return -1;
    buffer_t* g_vertex_buffer = std::get<0>(bufs.value());
    buffer_t* g_index_buffer = std::get<1>(bufs.value());
    
    buffer_settings_t vertex_buffer_settings;
    vertex_buffer_settings.populate_defaults(static_cast<std::uint32_t>(vertices.size()));
    if (vk_context.add_buffer(vertex_buffer_settings) != 0) return -1;
    buffer_t* vertex_buffer = vk_context.get_last_buffer();
    vertex_buffer->set_staged_data(vertices.data());
    
    buffer_settings_t index_buffer_settings;
    index_buffer_settings.populate_defaults(0, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    index_buffer_settings.size = static_cast<std::uint32_t>(indices.size()) * sizeof(std::uint32_t);
    if (vk_context.add_buffer(index_buffer_settings) != 0) return -1;
    buffer_t* index_buffer = vk_context.get_last_buffer();
    index_buffer->set_staged_data(indices.data());

    std::vector<std::tuple<std::uint32_t, VkDeviceSize, void*, VkDescriptorType, bool>> g_descriptor_config;
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
    g_descriptor_config.push_back(std::make_tuple(0, sizeof(ubo_t), &ubo_buffers[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, true));

    image_settings_t image_settings;
    if (vk_context.add_image(texture_path, image_settings, true) != 0) return -1;
    image_t* img = vk_context.get_image(0);
    g_descriptor_config.push_back(std::make_tuple(1, 0, &img, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, false));

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
    descriptor_config.push_back(std::make_tuple(3, sizeof(blinn_phong_t), &blinn_phong_buffers[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, true));

    std::vector<VkDescriptorSetLayoutBinding> g_bindings = { UBO_LAYOUT_BINDING, SAMPLER_LAYOUT_BINDING };

    VkDescriptorSetLayoutBinding g_pos_binding = { 0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    VkDescriptorSetLayoutBinding g_normal_binding = { 1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    VkDescriptorSetLayoutBinding g_albedo_binding = { 2, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    VkDescriptorSetLayoutBinding phong_layout_binding = { 3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    std::vector<VkDescriptorSetLayoutBinding> out_bindings = { g_pos_binding, g_normal_binding, g_albedo_binding, phong_layout_binding };

    
    std::vector<image_t*> g_buffer;
    image_settings_t g_buffer_settings;
    g_buffer_settings.sample_count = vk_context.msaa_samples;
    g_buffer_settings.format = vk_context.swap_chain->format.format;
    g_buffer_settings.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    // POS
    g_buffer.push_back(new image_t(&vk_context.physical_device, &vk_context.command_pool));
    g_buffer.back()->init_color_buffer(g_buffer_settings, vk_context.get_swap_chain_extent(), vk_context.device);
    vk_context.color_buffers.push_back(g_buffer.back());

    // NORMAL
    g_buffer.push_back(new image_t(&vk_context.physical_device, &vk_context.command_pool));
    g_buffer.back()->init_color_buffer(g_buffer_settings, vk_context.get_swap_chain_extent(), vk_context.device);
    vk_context.color_buffers.push_back(g_buffer.back());

    // ALBEDO
    g_buffer_settings.sample_count = VK_SAMPLE_COUNT_1_BIT;
    g_buffer_settings.usage = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    g_buffer.push_back(new image_t(&vk_context.physical_device, &vk_context.command_pool));
    g_buffer.back()->init_color_buffer(g_buffer_settings, vk_context.get_swap_chain_extent(), vk_context.device);
    g_buffer.back()->transition_image_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vk_context.color_buffers.push_back(g_buffer.back());
  
    image_settings_t g_depth_settings;
    g_depth_settings.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    g_depth_settings.sample_count = vk_context.msaa_samples;
    g_buffer.push_back(new image_t(&vk_context.physical_device, &vk_context.command_pool));
    g_buffer.back()->init_depth_buffer(g_depth_settings, vk_context.get_swap_chain_extent(), vk_context.device);
    vk_context.depth_buffers.push_back(g_buffer.back());

    // resolve buffers for the msaa
    image_settings_t resolve_buffer_settings;
    resolve_buffer_settings.sample_count = VK_SAMPLE_COUNT_1_BIT;
    resolve_buffer_settings.format = vk_context.swap_chain->format.format;
    resolve_buffer_settings.usage = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    g_buffer.push_back(new image_t(&vk_context.physical_device, &vk_context.command_pool));
    g_buffer.back()->init_color_buffer(resolve_buffer_settings, vk_context.get_swap_chain_extent(), vk_context.device);
    g_buffer.back()->transition_image_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vk_context.color_buffers.push_back(g_buffer.back());
    
    g_buffer.push_back(new image_t(&vk_context.physical_device, &vk_context.command_pool));
    g_buffer.back()->init_color_buffer(resolve_buffer_settings, vk_context.get_swap_chain_extent(), vk_context.device);
    g_buffer.back()->transition_image_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vk_context.color_buffers.push_back(g_buffer.back());
    
    g_buffer.push_back(new image_t(&vk_context.physical_device, &vk_context.command_pool));
    g_buffer.back()->init_color_buffer(resolve_buffer_settings, vk_context.get_swap_chain_extent(), vk_context.device);
    g_buffer.back()->transition_image_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vk_context.color_buffers.push_back(g_buffer.back());

    image_t** g_pos = &vk_context.color_buffers[3];
    descriptor_config.push_back(std::make_tuple(0, 0, g_pos, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, false));
    image_t** g_normal = &vk_context.color_buffers[4];
    descriptor_config.push_back(std::make_tuple(1, 0, g_normal, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, false));
    image_t** g_albedo = &vk_context.color_buffers[2];
    descriptor_config.push_back(std::make_tuple(2, 0, g_albedo, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, false));

    render_pass_settings_t render_pass_settings;
    render_pass_settings.add_subpass(vk_context.swap_chain->format.format, vk_context.msaa_samples, &vk_context.physical_device, 3, 1, 3);
    render_pass_settings.attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
    render_pass_settings.subpasses[0].color_attachment_resolve_references[2].attachment = VK_ATTACHMENT_UNUSED;
    render_pass_settings.add_subpass(vk_context.swap_chain->format.format, VK_SAMPLE_COUNT_1_BIT, &vk_context.physical_device, 1, 0, 0, 3, 4);
    render_pass_settings.subpasses[1].input_attachment_references[2].attachment = 2;
    render_pass_settings.attachments.back().finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    {
        VkSubpassDependency& dep = render_pass_settings.dependencies.back();
        dep.srcSubpass = VK_SUBPASS_EXTERNAL;
        dep.dstSubpass = 0;
        dep.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    }

    VkSubpassDependency dep{};
    dep.srcSubpass = 0;
    dep.dstSubpass = 1;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dep.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    render_pass_settings.dependencies.push_back(dep);

    render_pass_t* render_pass = new render_pass_t();
    render_pass->init(render_pass_settings, vk_context.device->device);
    for (std::uint32_t i = 0; i < vk_context.swap_chain->image_views.size(); ++i)
    {
        std::vector<framebuffer_attachment_t> attachments = { {&vk_context.color_buffers[0], IMAGE}, {&vk_context.color_buffers[1], IMAGE},
            {&vk_context.color_buffers[2], IMAGE}, {&vk_context.depth_buffers[0], IMAGE},{&vk_context.color_buffers[3], IMAGE},
            {&vk_context.color_buffers[4], IMAGE}, {&vk_context.color_buffers[5], IMAGE},
            /*{&vk_context.color_buffer, IMAGE}, {&vk_context.depth_buffer, IMAGE},*/ {&vk_context.swap_chain, SWAP_CHAIN, i} };
        render_pass->add_framebuffer(vk_context.swap_chain->extent.width, vk_context.swap_chain->extent.height, attachments);
    }
    vk_context.render_passes.push_back(render_pass);

    vk_context.add_descriptor_set_layout(g_bindings);
    pipeline_shaders_t g_shaders = { "./build/target/shaders/g_buffer.vert.spv", std::nullopt, "./build/target/shaders/g_buffer.frag.spv" };
    pipeline_settings_t g_pipeline_settings;
    g_pipeline_settings.populate_defaults(vk_context.get_descriptor_set_layouts(), vk_context.render_passes[0], 3);
    g_pipeline_settings.multisampling.rasterizationSamples = vk_context.msaa_samples;
    if (vk_context.add_pipeline(g_shaders, g_pipeline_settings) != 0) return -1;
    
    vk_context.add_descriptor_set_layout(out_bindings);
    pipeline_shaders_t shaders = { "./build/target/shaders/main.vert.spv", std::nullopt, "./build/target/shaders/main.frag.spv" };
    pipeline_settings_t pipeline_settings;
    pipeline_settings.populate_defaults({ vk_context.get_descriptor_set_layouts()[1] }, vk_context.render_passes[0]);
    pipeline_settings.subpass = 1;
    if (vk_context.add_pipeline(shaders, pipeline_settings) != 0) return -1;

    std::vector<descriptor_pool_t*> pools = *vk_context.get_descriptor_pools();
    pools[0]->configure_descriptors(g_descriptor_config);
    pools[1]->configure_descriptors(descriptor_config);

    std::function<void(VkCommandBuffer, vulkan_context_t*)> draw_command = [&] (VkCommandBuffer command_buffer, vulkan_context_t* context)
    {
        {
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->graphics_pipelines[0]->pipeline);
            context->current_pipeline = context->graphics_pipelines[0];
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

            VkBuffer vertex_buffers[] = { g_vertex_buffer->buffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
            vkCmdBindIndexBuffer(command_buffer, g_index_buffer->buffer, 0, VK_INDEX_TYPE_UINT32);
            context->bind_descriptor_sets(command_buffer, 0, 0);

            vkCmdDrawIndexed(command_buffer, static_cast<std::uint32_t>(model.indices.size()), 1, 0, 0, 0);
        }

        vkCmdNextSubpass(command_buffer, VK_SUBPASS_CONTENTS_INLINE);

        {
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->graphics_pipelines[1]->pipeline);
            context->current_pipeline = context->graphics_pipelines[1];
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
            context->bind_descriptor_sets(command_buffer, 1, 0);

            vkCmdDrawIndexed(command_buffer, static_cast<std::uint32_t>(indices.size()), 1, 0, 0, 0);
        }
    };

    vk_context.main_loop([&]
    {
        while (!glfwWindowShouldClose(vk_context.window))
        {
            glfwPollEvents();

            static std::chrono::time_point start_time = std::chrono::high_resolution_clock::now();
            std::chrono::time_point current_time = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();
            static blinn_phong_t blinn_phong = { {0.0f, 4.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {10.0f, 0.0f, 0.0f}, 0.7f, 1.8f };
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

    // std::for_each(g_buffer.begin(), g_buffer.end(), [] (image_t* img) { delete img; } );

    glfwTerminate();
    return 0;
}
