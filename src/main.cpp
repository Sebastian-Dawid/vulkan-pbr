#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include "vulkan_base/vulkan_base.h"
#include "vulkan_base/vulkan_vertex.h"
#include "model_base/model_base.h"
#include "command_line_parser/command_line_parser.h"
#include "user_base/camera.h"
#include <chrono>
#include <cstring>
#include <iostream>

struct ubo_t
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
};

struct shadow_map_t
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
    alignas(16) glm::vec3 light_pos;
};

struct view_t
{
    alignas(16) glm::vec3 pos;
    alignas(16) glm::mat4 mat;
};

struct flags_t
{
    alignas(4) bool normal_map;
};

struct blinn_phong_t
{
    alignas(16) glm::vec3 light_pos;
    alignas(16) glm::vec3 light_color;
    alignas(16) glm::vec3 ambient_color;
    alignas(16) glm::vec3 view_pos;

    alignas(4) float linear;
    alignas(4) float quadratic;
};

std::vector<vertex_t> vertices = {
    { { 1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {}, {1.0f, 1.0f} }, // top right
    { { 1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {}, {1.0f, 0.0f} }, // bottom right
    { {-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {}, {0.0f, 0.0f} }, // bottom left
    { {-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {}, {0.0f, 1.0f} }  // top left
};
std::vector<std::uint32_t> indices = { 0, 1, 2, 2, 3, 0 };

camera_t cam(glm::vec3(0.0f, 0.0f, 6.0f));
bool camera_active = false;
bool first_mouse = true;
float last_x = 0.0f, last_y = 0.0f;

void mouse_button_callback(GLFWwindow* window, int button, int action, int mode)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) 
    {
        camera_active = true;
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) camera_active = false;
}

void cursor_pos_callback(GLFWwindow* window, double x_pos, double y_pos)
{
    if (first_mouse)
    {
        last_x = x_pos;
        last_y = y_pos;
        first_mouse = false;
    }
    if (camera_active) cam.turn(x_pos - last_x, last_y - y_pos);
    last_x = x_pos;
    last_y = y_pos;
}

void scroll_callback(GLFWwindow* window, double x_offset, double y_offset)
{
    if (camera_active) cam.zoom(y_offset);
}

void imgui_add_dependency(std::uint32_t subpass, std::uint32_t attachment, vulkan_context_t* vk_context, render_pass_settings_t& settings)
{
    settings.add_subpass(vk_context->swap_chain->format.format, VK_SAMPLE_COUNT_1_BIT, &vk_context->physical_device);
    settings.attachments.pop_back();
    settings.subpasses.back().color_attachment_references.back().attachment = attachment;
    VkSubpassDependency& imgui_dep = settings.dependencies.back();
    imgui_dep.srcSubpass = subpass - 1;
    imgui_dep.dstSubpass = subpass;
    imgui_dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    imgui_dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    imgui_dep.srcAccessMask = 0;
    imgui_dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
}

VkDescriptorPool imgui_setup(std::uint32_t subpass, vulkan_context_t* vk_context)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    VkDescriptorPoolSize pool_sizes[] = { {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1} };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = (std::uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    VkDescriptorPool imgui_pool;
    vkCreateDescriptorPool(vk_context->device->device, &pool_info, nullptr, &imgui_pool);

    ImGui_ImplGlfw_InitForVulkan(vk_context->window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = vk_context->instance;
    init_info.PhysicalDevice = vk_context->physical_device;
    init_info.Device = vk_context->device->device;
    init_info.QueueFamily = vk_context->device->indices.graphics_family.value();
    init_info.Queue = vk_context->device->graphics_queue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = imgui_pool;
    init_info.Subpass = subpass;
    init_info.MinImageCount = MAX_FRAMES_IN_FLIGHT;
    init_info.ImageCount = vk_context->swap_chain->images.size();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = [](VkResult err){ if (err == 0) return; fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err); if (err < 0) abort(); };
    ImGui_ImplVulkan_Init(&init_info, vk_context->render_passes.back()->render_pass);

    {
        VkCommandBuffer buf = begin_single_time_commands(vk_context->device->device, vk_context->command_pool);
        ImGui_ImplVulkan_CreateFontsTexture(buf);
        end_single_time_commands(vk_context->command_pool, buf, vk_context->device->device, vk_context->device->graphics_queue);
        vkDeviceWaitIdle(vk_context->device->device);
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
    return imgui_pool;
}

int main(int argc, char** argv)
{

    std::uint32_t width = 1280, height = 720;
    bool flip_texture = true;
    std::string model_path = "./models/backpack/backpack.obj", albedo_path = "./models/backpack/albedo.jpg", specular_path = "./models/backpack/specular.jpg",
        normal_path = "./models/backpack/normal.png", metallic_path = "./models/backpack/metallic.jpg", roughness_path = "./models/backpack/roughness.jpg",
        ao_path = "./models/backpack/ao.jpg";
    if (argc != 1)
    {
        std::map<std::string, std::tuple<std::vector<value_type_t>, std::uint32_t>> allowed_args;
        allowed_args["--help"] = std::make_tuple(std::vector<value_type_t>{ value_type_t::NONE }, 0);
        allowed_args["--width"] = std::make_tuple(std::vector<value_type_t>{ value_type_t::UINT }, 1);
        allowed_args["--height"] = std::make_tuple(std::vector<value_type_t>{ value_type_t::UINT }, 1);
        allowed_args["--model"] = std::make_tuple(std::vector<value_type_t>{ value_type_t::STRING }, 1);
        allowed_args["--texture"] = std::make_tuple(std::vector<value_type_t>{ value_type_t::STRING }, 1);
        allowed_args["--flip-texture"] = std::make_tuple(std::vector<value_type_t>{ value_type_t::NONE }, 0);
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
            albedo_path = res["--texture"][0].s;
        }
        if (std::find_if(res.begin(), res.end(), [](auto e){ return std::strcmp(e.first.c_str(), "--flip-texture") == 0; }) != res.end())
        {
            flip_texture = false;
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

    glfwSetMouseButtonCallback(vk_context.window, mouse_button_callback);
    glfwSetCursorPosCallback(vk_context.window, cursor_pos_callback);
    glfwSetScrollCallback(vk_context.window, scroll_callback);

    model_t model(model_path, false);
    if (!model.is_initialized()) return -1;
    auto bufs = model.set_up_buffer(&vk_context);
    if (!bufs.has_value()) return -1;
    buffer_t* g_vertex_buffer = std::get<0>(bufs.value());
    buffer_t* g_index_buffer = std::get<1>(bufs.value());
    
    model_t cube("./models/cube/cube.obj", false);
    if (!cube.is_initialized()) return -1;
    bufs = cube.set_up_buffer(&vk_context);
    if (!bufs.has_value()) return -1;
    buffer_t* forward_vertex_buffer = std::get<0>(bufs.value());
    buffer_t* forward_index_buffer = std::get<1>(bufs.value());
    
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
    std::vector<std::tuple<std::uint32_t, VkDeviceSize, void*, VkDescriptorType, bool>> forward_descriptor_config;
    std::vector<std::tuple<std::uint32_t, VkDeviceSize, void*, VkDescriptorType, bool>> hdr_descriptor_config;
    std::vector<std::tuple<std::uint32_t, VkDeviceSize, void*, VkDescriptorType, bool>> shadow_map_descriptor_config;

    std::vector<buffer_t*> ubo_buffers;
    for (std::uint32_t i = 0; i < 2 * MAX_FRAMES_IN_FLIGHT; ++i)
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
    for (std::uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        buffer_settings_t ubo_settings;
        ubo_settings.size = sizeof(flags_t);
        ubo_settings.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        ubo_settings.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if (vk_context.add_buffer(ubo_settings) != 0) return -1;
        buffer_t* ubo_buffer = vk_context.get_last_buffer();
        ubo_buffer->map_memory();
        ubo_buffers.push_back(ubo_buffer);
    }
    for (std::uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        buffer_settings_t ubo_settings;
        ubo_settings.size = sizeof(view_t);
        ubo_settings.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        ubo_settings.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if (vk_context.add_buffer(ubo_settings) != 0) return -1;
        buffer_t* ubo_buffer = vk_context.get_last_buffer();
        ubo_buffer->map_memory();
        ubo_buffers.push_back(ubo_buffer);
    }
    for (std::uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
         buffer_settings_t ubo_settings;
         ubo_settings.size = sizeof(shadow_map_t);
         ubo_settings.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
         ubo_settings.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
         if (vk_context.add_buffer(ubo_settings) != 0) return -1;
         buffer_t* ubo_buffer = vk_context.get_last_buffer();
         ubo_buffer->map_memory();
         ubo_buffers.push_back(ubo_buffer);
    }
    g_descriptor_config.push_back(std::make_tuple(0, sizeof(ubo_t), &ubo_buffers[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, true));
    g_descriptor_config.push_back(std::make_tuple(7, sizeof(flags_t), &ubo_buffers[MAX_FRAMES_IN_FLIGHT * 2], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, true));
    descriptor_config.push_back(std::make_tuple(5, sizeof(view_t), &ubo_buffers[MAX_FRAMES_IN_FLIGHT * 3], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, true));
    forward_descriptor_config.push_back(std::make_tuple(0, sizeof(ubo_t), &ubo_buffers[MAX_FRAMES_IN_FLIGHT], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, true));
    shadow_map_descriptor_config.push_back(std::make_tuple(0, sizeof(shadow_map_t), &ubo_buffers[MAX_FRAMES_IN_FLIGHT * 4], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, true));

    image_settings_t image_settings;
    image_settings.format = VK_FORMAT_R8G8B8A8_UNORM;
    if (vk_context.add_image(albedo_path, image_settings, flip_texture) != 0) return -1;
    if (vk_context.add_image(specular_path, image_settings, flip_texture) != 0) return -1;
    if (vk_context.add_image(normal_path, image_settings, flip_texture) != 0) return -1;
    if (vk_context.add_image(metallic_path, image_settings, flip_texture) != 0) return -1;
    if (vk_context.add_image(roughness_path, image_settings, flip_texture) != 0) return -1;
    if (vk_context.add_image(ao_path, image_settings, flip_texture) != 0) return -1;
    
    g_descriptor_config.push_back(std::make_tuple(1, 0, &vk_context.images[0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, false));
    g_descriptor_config.push_back(std::make_tuple(2, 0, &vk_context.images[1], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, false));
    g_descriptor_config.push_back(std::make_tuple(3, 0, &vk_context.images[2], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, false));
    g_descriptor_config.push_back(std::make_tuple(4, 0, &vk_context.images[3], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, false));
    g_descriptor_config.push_back(std::make_tuple(5, 0, &vk_context.images[4], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, false));
    g_descriptor_config.push_back(std::make_tuple(6, 0, &vk_context.images[5], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, false));

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
    descriptor_config.push_back(std::make_tuple(4, sizeof(blinn_phong_t), &blinn_phong_buffers[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, true));

    VkDescriptorSetLayoutBinding spec_binding = SAMPLER_LAYOUT_BINDING;
    spec_binding.binding = 2;
    VkDescriptorSetLayoutBinding normal_binding = SAMPLER_LAYOUT_BINDING;
    normal_binding.binding = 3;
    VkDescriptorSetLayoutBinding metallic_binding = SAMPLER_LAYOUT_BINDING;
    metallic_binding.binding = 4;
    VkDescriptorSetLayoutBinding roughness_binding = SAMPLER_LAYOUT_BINDING;
    roughness_binding.binding = 5;
    VkDescriptorSetLayoutBinding ao_binding = SAMPLER_LAYOUT_BINDING;
    ao_binding.binding = 6;
    VkDescriptorSetLayoutBinding flags_binding = UBO_LAYOUT_BINDING;
    flags_binding.binding = 7;
    flags_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    std::vector<VkDescriptorSetLayoutBinding> g_bindings = { UBO_LAYOUT_BINDING, SAMPLER_LAYOUT_BINDING, spec_binding, normal_binding, metallic_binding,
        roughness_binding, ao_binding, flags_binding};

    VkDescriptorSetLayoutBinding g_pos_binding = { 0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    VkDescriptorSetLayoutBinding g_normal_binding = { 1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    VkDescriptorSetLayoutBinding g_albedo_binding = { 2, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    VkDescriptorSetLayoutBinding g_pbr_binding = { 3, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    VkDescriptorSetLayoutBinding phong_layout_binding = { 4, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    VkDescriptorSetLayoutBinding view_layout_binding = { 5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
    std::vector<VkDescriptorSetLayoutBinding> out_bindings = { g_pos_binding, g_normal_binding, g_albedo_binding, g_pbr_binding, phong_layout_binding, view_layout_binding };

    std::vector<VkDescriptorSetLayoutBinding> forward_bindings = { UBO_LAYOUT_BINDING };
    std::vector<VkDescriptorSetLayoutBinding> hdr_bindings = { {0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr} };
    std::vector<VkDescriptorSetLayoutBinding> shadow_map_bindings = { UBO_LAYOUT_BINDING };

    /* SHADOW MAP RENDER PASS AND PIPELINE */
    image_settings_t shadow_map_settings;
    shadow_map_settings.format = VK_FORMAT_R32_SFLOAT;
    shadow_map_settings.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    shadow_map_settings.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    shadow_map_settings.layer_count = 6;
    image_view_settings_t shadow_map_view_settings = {
        .type = VK_IMAGE_VIEW_TYPE_CUBE,
        .format = VK_FORMAT_R32_SFLOAT,
        .layer_count = 6,
        .components = { VK_COMPONENT_SWIZZLE_R }
    };
    image_t* shadow_map = new image_t(&vk_context.physical_device, &vk_context.command_pool);
    shadow_map->init_color_buffer(shadow_map_settings, { 1024, 1024 }, vk_context.device, shadow_map_view_settings);
    shadow_map->transition_image_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    sampler_settings_t shadow_sampler_settings;
    shadow_map->create_image_sampler(shadow_sampler_settings);

    render_pass_settings_t shadow_map_pass_settings;
    shadow_map_pass_settings.add_subpass(VK_FORMAT_R32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, &vk_context.physical_device);

    render_pass_t* shadow_map_pass = new render_pass_t();
    shadow_map_pass->init(shadow_map_pass_settings, vk_context.device->device);
    std::vector<framebuffer_attachment_t> shadow_map_attachments = { {&shadow_map, IMAGE} };
    shadow_map_pass->add_framebuffer(1024, 1024, shadow_map_attachments);
    shadow_map_pass->resizeable = false;
    vk_context.render_passes.push_back(shadow_map_pass);

    vk_context.add_descriptor_set_layout(shadow_map_bindings);
    pipeline_shaders_t shadow_map_shaders = { "./build/target/shaders/shadow_map.vert.spv", std::nullopt, "./build/target/shaders/shadow_map.frag.spv" };
    pipeline_settings_t shadow_map_pipeline_settings;
    shadow_map_pipeline_settings.populate_defaults(vk_context.get_descriptor_set_layouts(), vk_context.render_passes[0]);
    if (vk_context.add_pipeline(shadow_map_shaders, shadow_map_pipeline_settings) != 0) return -1;

    /* DEFFERED PBR RENDER PASS AND PIPELINES */
    std::vector<image_t*> g_buffer;
    image_settings_t g_buffer_settings;
    g_buffer_settings.sample_count = VK_SAMPLE_COUNT_1_BIT;
    g_buffer_settings.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    g_buffer_settings.usage = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    // POS
    g_buffer.push_back(new image_t(&vk_context.physical_device, &vk_context.command_pool));
    g_buffer.back()->init_color_buffer(g_buffer_settings, vk_context.get_swap_chain_extent(), vk_context.device);
    g_buffer.back()->transition_image_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vk_context.color_buffers.push_back(g_buffer.back());

    // NORMAL
    g_buffer.push_back(new image_t(&vk_context.physical_device, &vk_context.command_pool));
    g_buffer.back()->init_color_buffer(g_buffer_settings, vk_context.get_swap_chain_extent(), vk_context.device);
    g_buffer.back()->transition_image_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vk_context.color_buffers.push_back(g_buffer.back());

    // ALBEDO
    g_buffer.push_back(new image_t(&vk_context.physical_device, &vk_context.command_pool));
    g_buffer.back()->init_color_buffer(g_buffer_settings, vk_context.get_swap_chain_extent(), vk_context.device);
    g_buffer.back()->transition_image_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vk_context.color_buffers.push_back(g_buffer.back());
 
    // PBR
    g_buffer.push_back(new image_t(&vk_context.physical_device, &vk_context.command_pool));
    g_buffer.back()->init_color_buffer(g_buffer_settings, vk_context.get_swap_chain_extent(), vk_context.device);
    g_buffer.back()->transition_image_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vk_context.color_buffers.push_back(g_buffer.back());

    image_settings_t hdr_buffer_settings;
    hdr_buffer_settings.sample_count = VK_SAMPLE_COUNT_1_BIT;
    hdr_buffer_settings.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    hdr_buffer_settings.usage = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_t* hdr_buffer = new image_t(&vk_context.physical_device, &vk_context.command_pool);
    hdr_buffer->init_color_buffer(hdr_buffer_settings, vk_context.get_swap_chain_extent(), vk_context.device);
    hdr_buffer->transition_image_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vk_context.color_buffers.push_back(hdr_buffer);

    image_settings_t g_depth_settings;
    g_depth_settings.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    g_depth_settings.sample_count = VK_SAMPLE_COUNT_1_BIT;
    g_buffer.push_back(new image_t(&vk_context.physical_device, &vk_context.command_pool));
    g_buffer.back()->init_depth_buffer(g_depth_settings, vk_context.get_swap_chain_extent(), vk_context.device);
    vk_context.depth_buffers.push_back(g_buffer.back());

    image_t** g_pos = &vk_context.color_buffers[0];
    descriptor_config.push_back(std::make_tuple(0, 0, g_pos, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, false));
    image_t** g_normal = &vk_context.color_buffers[1];
    descriptor_config.push_back(std::make_tuple(1, 0, g_normal, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, false));
    image_t** g_albedo = &vk_context.color_buffers[2];
    descriptor_config.push_back(std::make_tuple(2, 0, g_albedo, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, false));
    image_t** g_pbr = &vk_context.color_buffers[3];
    descriptor_config.push_back(std::make_tuple(3, 0, g_pbr, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, false));
    image_t** hdr = &vk_context.color_buffers[4];
    hdr_descriptor_config.push_back(std::make_tuple(0, 0, hdr, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, false));
    
    render_pass_settings_t render_pass_settings;
    render_pass_settings.add_subpass(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_1_BIT, &vk_context.physical_device, 4, 1, 0);
    render_pass_settings.add_subpass(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_1_BIT, &vk_context.physical_device, 1, 0, 0, 4);
    render_pass_settings.add_subpass(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLE_COUNT_1_BIT, &vk_context.physical_device, 1, 1, 0, 0);
    // remove added color and depth attachments
    render_pass_settings.attachments.pop_back();
    render_pass_settings.attachments.pop_back();
    render_pass_settings.subpasses.back().depth_attachment_reference.back().attachment = 4;
    render_pass_settings.subpasses.back().color_attachment_references.back().attachment = 5;
    render_pass_settings.add_subpass(vk_context.swap_chain->format.format, VK_SAMPLE_COUNT_1_BIT, &vk_context.physical_device, 1, 0, 0, 1, 5);
    render_pass_settings.attachments.back().finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    render_pass_settings.dependencies.clear();
    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dep.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    dep.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    render_pass_settings.dependencies.push_back(dep);

    dep = {};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    render_pass_settings.dependencies.push_back(dep);

    dep = {};
    dep.srcSubpass = 0;
    dep.dstSubpass = 1;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dep.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    render_pass_settings.dependencies.push_back(dep);

    dep = {};
    dep.srcSubpass = 1;
    dep.dstSubpass = 2;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    render_pass_settings.dependencies.push_back(dep);

    dep.srcSubpass = 2;
    dep.dstSubpass = 3;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    render_pass_settings.dependencies.push_back(dep);
    
    dep = {};
    dep.srcSubpass = 0;
    dep.dstSubpass = 2;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    render_pass_settings.dependencies.push_back(dep);

    dep = {};
    dep.srcSubpass = 3;
    dep.dstSubpass = VK_SUBPASS_EXTERNAL;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dep.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    render_pass_settings.dependencies.push_back(dep);

    imgui_add_dependency(4, 6, &vk_context, render_pass_settings);

    render_pass_t* render_pass = new render_pass_t();
    render_pass->init(render_pass_settings, vk_context.device->device);
    for (std::uint32_t i = 0; i < vk_context.swap_chain->image_views.size(); ++i)
    {
        std::vector<framebuffer_attachment_t> attachments = { {&vk_context.color_buffers[0], IMAGE}, {&vk_context.color_buffers[1], IMAGE},
            {&vk_context.color_buffers[2], IMAGE}, {&vk_context.color_buffers[3]}, {&vk_context.depth_buffers[0], IMAGE},
            {&vk_context.color_buffers[4]}, {&vk_context.swap_chain, SWAP_CHAIN, i} };
        render_pass->add_framebuffer(vk_context.swap_chain->extent.width, vk_context.swap_chain->extent.height, attachments);
    }
    vk_context.render_passes.push_back(render_pass);

    vk_context.add_descriptor_set_layout(g_bindings);
    pipeline_shaders_t g_shaders = { "./build/target/shaders/g_buffer.vert.spv", std::nullopt, "./build/target/shaders/g_buffer.frag.spv" };
    pipeline_settings_t g_pipeline_settings;
    g_pipeline_settings.populate_defaults({ vk_context.get_descriptor_set_layouts()[1] }, vk_context.render_passes[1], 4);
    //g_pipeline_settings.multisampling.rasterizationSamples = vk_context.msaa_samples;
    if (vk_context.add_pipeline(g_shaders, g_pipeline_settings) != 0) return -1;
    
    vk_context.add_descriptor_set_layout(out_bindings);
    pipeline_shaders_t shaders = { "./build/target/shaders/main.vert.spv", std::nullopt, "./build/target/shaders/main.frag.spv" };
    pipeline_settings_t pipeline_settings;
    pipeline_settings.populate_defaults({ vk_context.get_descriptor_set_layouts()[2] }, vk_context.render_passes[1]);
    pipeline_settings.subpass = 1;
    if (vk_context.add_pipeline(shaders, pipeline_settings) != 0) return -1;

    vk_context.add_descriptor_set_layout(forward_bindings);
    pipeline_shaders_t forward_shaders = { "./build/target/shaders/forward.vert.spv", std::nullopt, "./build/target/shaders/forward.frag.spv" };
    pipeline_settings_t forward_pipeline_settings;
    forward_pipeline_settings.populate_defaults({ vk_context.get_descriptor_set_layouts()[3] }, vk_context.render_passes[1]);
    forward_pipeline_settings.subpass = 2;
    if (vk_context.add_pipeline(forward_shaders, forward_pipeline_settings) != 0) return -1;

    vk_context.add_descriptor_set_layout(hdr_bindings);
    pipeline_shaders_t hdr_shaders = { "./build/target/shaders/hdr.vert.spv", std::nullopt, "./build/target/shaders/hdr.frag.spv" };
    pipeline_settings_t hdr_pipeline_settings;
    hdr_pipeline_settings.populate_defaults({ vk_context.get_descriptor_set_layouts()[4] }, vk_context.render_passes[1]);
    hdr_pipeline_settings.subpass = 3;
    if (vk_context.add_pipeline(hdr_shaders, hdr_pipeline_settings) != 0) return -1;

    std::vector<descriptor_pool_t*> pools = *vk_context.get_descriptor_pools();
    pools[0]->configure_descriptors(shadow_map_descriptor_config);
    pools[1]->configure_descriptors(g_descriptor_config);
    pools[2]->configure_descriptors(descriptor_config);
    pools[3]->configure_descriptors(forward_descriptor_config);
    pools[4]->configure_descriptors(hdr_descriptor_config);

    VkDescriptorPool imgui_pool = imgui_setup(4, &vk_context);
    ImDrawData* draw_data;
    std::function<void(VkCommandBuffer, std::uint32_t, vulkan_context_t*)> draw_command = [&] (VkCommandBuffer command_buffer, std::uint32_t image_index, vulkan_context_t* context)
    {
        VkRenderPassBeginInfo begin_info = populate_render_pass_begin_info(context->render_passes[0]->render_pass, context->render_passes[0]->framebuffers[0].framebuffer,
                {1024, 1024}, G_CLEAR_COLORS);
        vkCmdBeginRenderPass(command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
        {
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->graphics_pipelines[0]->pipeline);
            context->current_pipeline = context->graphics_pipelines[0];
            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(1024);
            viewport.height = static_cast<float>(1024);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(command_buffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = { 1024, 1024 };
            vkCmdSetScissor(command_buffer, 0, 1, &scissor);

            VkBuffer vertex_buffers[] = { g_vertex_buffer->buffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
            vkCmdBindIndexBuffer(command_buffer, g_index_buffer->buffer, 0, VK_INDEX_TYPE_UINT32);
            context->bind_descriptor_sets(command_buffer, 1, 0);

            vkCmdDrawIndexed(command_buffer, static_cast<std::uint32_t>(model.indices.size()), 1, 0, 0, 0);
        }
        vkCmdEndRenderPass(command_buffer);

        begin_info = populate_render_pass_begin_info(context->render_passes[1]->render_pass, context->render_passes[1]->framebuffers[image_index].framebuffer,
                context->get_swap_chain_extent(), G_CLEAR_COLORS);
        vkCmdBeginRenderPass(command_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
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

            VkBuffer vertex_buffers[] = { g_vertex_buffer->buffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
            vkCmdBindIndexBuffer(command_buffer, g_index_buffer->buffer, 0, VK_INDEX_TYPE_UINT32);
            context->bind_descriptor_sets(command_buffer, 1, 0);

            vkCmdDrawIndexed(command_buffer, static_cast<std::uint32_t>(model.indices.size()), 1, 0, 0, 0);
        }

        vkCmdNextSubpass(command_buffer, VK_SUBPASS_CONTENTS_INLINE);

        {
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->graphics_pipelines[2]->pipeline);
            context->current_pipeline = context->graphics_pipelines[2];
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
            context->bind_descriptor_sets(command_buffer, 2, 0);

            vkCmdDrawIndexed(command_buffer, static_cast<std::uint32_t>(indices.size()), 1, 0, 0, 0);
        }

        vkCmdNextSubpass(command_buffer, VK_SUBPASS_CONTENTS_INLINE);

        {
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->graphics_pipelines[3]->pipeline);
            context->current_pipeline = context->graphics_pipelines[3];
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

            VkBuffer vertex_buffers[] = { forward_vertex_buffer->buffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
            vkCmdBindIndexBuffer(command_buffer, forward_index_buffer->buffer, 0, VK_INDEX_TYPE_UINT32);
            context->bind_descriptor_sets(command_buffer, 3, 0);
            
            vkCmdDrawIndexed(command_buffer, static_cast<std::uint32_t>(cube.indices.size()), 1, 0, 0, 0);
        }

        vkCmdNextSubpass(command_buffer, VK_SUBPASS_CONTENTS_INLINE);

        {
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->graphics_pipelines[4]->pipeline);
            context->current_pipeline = context->graphics_pipelines[4];
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
            context->bind_descriptor_sets(command_buffer, 4, 0);

            vkCmdDrawIndexed(command_buffer, static_cast<std::uint32_t>(indices.size()), 1, 0, 0, 0);
        }

        vkCmdNextSubpass(command_buffer, VK_SUBPASS_CONTENTS_INLINE);
        ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer);
        vkCmdEndRenderPass(command_buffer);
    };

    vk_context.main_loop([&]
    {
        float delta_time = 0.0f;
        float last_frame = 0.0f;
        bool running = true;
        while (running && !glfwWindowShouldClose(vk_context.window))
        {
            glfwPollEvents();
            float current_frame = glfwGetTime();
            delta_time = current_frame - last_frame;
            last_frame = current_frame;

            if (glfwGetKey(vk_context.window, GLFW_KEY_ESCAPE) == GLFW_PRESS) running = false;//glfwWindowShouldClose(vk_context.window);
            if (glfwGetKey(vk_context.window, GLFW_KEY_W) == GLFW_PRESS) cam.move(FORWARD, delta_time);
            if (glfwGetKey(vk_context.window, GLFW_KEY_A) == GLFW_PRESS) cam.move(LEFT, delta_time);
            if (glfwGetKey(vk_context.window, GLFW_KEY_S) == GLFW_PRESS) cam.move(BACKWARD, delta_time);
            if (glfwGetKey(vk_context.window, GLFW_KEY_D) == GLFW_PRESS) cam.move(RIGHT, delta_time);

            static std::chrono::time_point start_time = std::chrono::high_resolution_clock::now();
            std::chrono::time_point current_time = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();
            static blinn_phong_t blinn_phong = { {0.0f, 0.0f, 3.0f}, {.2f, .2f, .6f}, {.02f, .02f, .06f}, {10.0f, 0.0f, 0.0f}, 0.09f, 0.032f };
            static float scale = 0.1;
            blinn_phong.view_pos = cam.position;
            static ubo_t ubo;
            time = 0.0f;
            ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            ubo.view =  cam.calculate_view_matrix();
            ubo.projection = glm::perspective(glm::radians(cam.fov_angle), vk_context.get_swap_chain_extent().width / (float) vk_context.get_swap_chain_extent().height, 0.1f, 100.0f);
            ubo.projection[1][1] *= -1;
            std::memcpy(ubo_buffers[vk_context.get_current_frame()]->mapped_memory, &ubo, sizeof(ubo_t));
            ubo.model = glm::translate(glm::mat4(1.0f), blinn_phong.light_pos);
            ubo.model = glm::scale(ubo.model, glm::vec3(scale, scale, scale));
            std::memcpy(ubo_buffers[MAX_FRAMES_IN_FLIGHT + vk_context.get_current_frame()]->mapped_memory, &ubo, sizeof(ubo_t));
            static flags_t flags = { true };
            std::memcpy(ubo_buffers[2 * MAX_FRAMES_IN_FLIGHT + vk_context.get_current_frame()]->mapped_memory, &flags, sizeof(flags_t));
            static view_t view;
            view.pos = cam.position;
            view.mat = ubo.view;
            std::memcpy(ubo_buffers[3 * MAX_FRAMES_IN_FLIGHT + vk_context.get_current_frame()]->mapped_memory, &view, sizeof(view_t));
            static shadow_map_t shadow_map_ubo;
            shadow_map_ubo.model = ubo.model;
            shadow_map_ubo.view = glm::lookAt(blinn_phong.light_pos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            shadow_map_ubo.projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
            shadow_map_ubo.projection[1][1] *= -1;
            shadow_map_ubo.light_pos = blinn_phong.light_pos;
            std::memcpy(ubo_buffers[4 * MAX_FRAMES_IN_FLIGHT + vk_context.get_current_frame()]->mapped_memory, &shadow_map_ubo, sizeof(shadow_map_t));

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            //ImGui::ShowDemoWindow();
            {
                ImGuiIO& io = ImGui::GetIO();
                ImGui::Begin("Settings");
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
                ImGui::SliderFloat3("light pos", &blinn_phong.light_pos.r, -5.0f, 5.0f);
                ImGui::SliderFloat("scale", &scale, 0.01f, 0.5f);
                ImGui::ColorEdit3("light color", &blinn_phong.light_color.r, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
                ImGui::ColorEdit3("ambient light color", &blinn_phong.ambient_color.r, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
                ImGui::End();
            }

            std::memcpy(blinn_phong_buffers[vk_context.get_current_frame()]->mapped_memory, &blinn_phong, sizeof(blinn_phong_t));

            ImGui::Render();
            draw_data = ImGui::GetDrawData();

            if (vk_context.draw_frame(draw_command) != 0)
            {
                break;
            }
        }
    });

    delete shadow_map;
    vkDestroyDescriptorPool(vk_context.device->device, imgui_pool, nullptr);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}
