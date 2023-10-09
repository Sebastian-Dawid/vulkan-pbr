#include "vulkan_base.h"
#include "vulkan_constants.h"
#include "vulkan_queue_family_indices.h"
#include "vulkan_validation_layers.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <set>

std::vector<const char*> get_required_extensions()
{
    std::uint32_t glfw_ext_count = 0;
    const char** p_glfw_exts;

    p_glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
    std::vector<const char*> glfw_exts(p_glfw_exts, p_glfw_exts + glfw_ext_count);
#ifdef PORTABILITY
    glfw_exts.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
    if (ENABLE_VALIDATION_LAYERS)
    {
        glfw_exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return glfw_exts;
}

std::int32_t vulkan_context_t::create_instance(std::string name)
{
    if (ENABLE_VALIDATION_LAYERS && !check_validation_layer_support())
    {
        std::cerr << "Validation layers requested, but not available!" << std::endl;
        return -1;
    }

    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = name.c_str();
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    std::vector<const char*> glfw_exts = get_required_extensions();

    create_info.enabledExtensionCount = glfw_exts.size();
    create_info.ppEnabledExtensionNames = glfw_exts.data();
#ifdef PORTABILITY
    create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    std::uint32_t ext_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr);
    std::vector<VkExtensionProperties> exts(ext_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, exts.data());

    std::cout << "Available Extentions:" << std::endl;
    for (const VkExtensionProperties& ext : exts)
    {
        std::cout << "\t" << ext.extensionName << std::endl;
    }

    std::cout << "Required Extentions:" << std::endl;
    for (const char* ext : glfw_exts)
    {
        std::cout << "\t" << ext << std::endl;
    }

    std::function<bool()> check_for_required_exts = [&]
    {
        for (const char* glfw_ext : glfw_exts)
        {
            if (std::find_if(exts.begin(), exts.end(), [&] (const VkExtensionProperties& in) { return std::strcmp(in.extensionName, glfw_ext) == 0; }) == exts.end())
            {
                return false;
            }
        }
        return true;
    };

    if (!check_for_required_exts())
    {
        std::cerr << "Required extention(s) not available!" << std::endl;
        return -1;
    }

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
    if (ENABLE_VALIDATION_LAYERS)
    {
        create_info.enabledLayerCount = static_cast<std::uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();
        std::cout << "Enabled Validation Layers:" << std::endl;
        for (const char* layer_name : validation_layers)
        {
            std::cout << "\t" << layer_name << std::endl;
        }

        populate_debug_messenger_create_info(debug_create_info);
        create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debug_create_info;
    }
    else
    {
        create_info.enabledLayerCount = 0;
        create_info.pNext = nullptr;
    }

    if (vkCreateInstance(&create_info, nullptr, &(this->instance)) != VK_SUCCESS)
    {
        std::cerr << "Failed to create instance!" << std::endl;
        return -1;
    }

    return 0;
}

bool check_physical_device_extension_support(VkPhysicalDevice physical_device)
{
    std::uint32_t ext_count = 0;
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &ext_count, nullptr);
    std::vector<VkExtensionProperties> available_extensions(ext_count);
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &ext_count, available_extensions.data());
   
    std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());
    for (const VkExtensionProperties& ext : available_extensions)
    {
        required_extensions.erase(ext.extensionName);
    }

    return required_extensions.empty();
}

std::uint32_t rate_device_suitablity(const VkPhysicalDevice& physical_device, VkSurfaceKHR& surface)
{
    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceProperties(physical_device, &device_properties);
    vkGetPhysicalDeviceFeatures(physical_device, &device_features);
   
    std::cout << "\t" << device_properties.deviceName << std::endl;
    std::cout << "\t\t" << "Device ID: " << device_properties.deviceID << std::endl;
    std::cout << "\t\t" << "Vendor ID: " << device_properties.vendorID << std::endl;
    std::cout << "\t\t" << "GPU Type: ";
    switch (device_properties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            std::cout << "Other GPU" << std::endl;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            std::cout << "Integrated GPU" << std::endl;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            std::cout << "Discrete GPU" << std::endl;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            std::cout << "Virtual GPU" << std::endl;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            std::cout << "CPU" << std::endl;
            break;
        default:
            std::cout << std::endl;
    }
    
    std::int32_t score = 0;

    if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 1000;
    }

    score += device_properties.limits.maxImageDimension2D;

    if (!device_features.geometryShader || !device_features.samplerAnisotropy)
    {
        return 0;
    }

    bool exts_supported = check_physical_device_extension_support(physical_device);

    queue_family_indices_t indices(physical_device, surface);
    if (!indices.is_complete() || !exts_supported)
    {
        return 0;
    }

    swap_chain_t swap_chain(physical_device, surface);
    if (!swap_chain.is_adequate())
    {
        return 0;
    }

    return score;
}

std::int32_t vulkan_context_t::pick_physical_device()
{
    std::uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(this->instance, &device_count, nullptr);
    std::cout << "Number of physical devices: " << device_count << std::endl;
    if (device_count == 0)
    {
        std::cerr << "Failed to find GPUs with Vulkan support!" << std::endl;
        return -1;
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(this->instance, &device_count, devices.data());

    std::multimap<std::uint32_t, VkPhysicalDevice> candidates;
    
    std::cout << "Available Devices: " << std::endl;
    for (const VkPhysicalDevice& device : devices)
    {
        std::uint32_t score = rate_device_suitablity(device, this->surface);
        candidates.insert(std::make_pair(score, device));
    }

    if (candidates.rbegin()->first > 0)
    {
        this->physical_device = candidates.rbegin()->second;
        std::cout << "Chosen GPU: " << std::endl;
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(this->physical_device, &props);
        std::cout << "\t" << props.deviceName << std::endl;

        VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts;
        if (counts & VK_SAMPLE_COUNT_64_BIT) this->msaa_samples = VK_SAMPLE_COUNT_64_BIT;
        else if (counts & VK_SAMPLE_COUNT_32_BIT) this->msaa_samples = VK_SAMPLE_COUNT_32_BIT;
        else if (counts & VK_SAMPLE_COUNT_16_BIT) this->msaa_samples = VK_SAMPLE_COUNT_16_BIT;
        else if (counts & VK_SAMPLE_COUNT_8_BIT) this->msaa_samples = VK_SAMPLE_COUNT_8_BIT;
        else if (counts & VK_SAMPLE_COUNT_4_BIT) this->msaa_samples = VK_SAMPLE_COUNT_4_BIT;
        else if (counts & VK_SAMPLE_COUNT_2_BIT) this->msaa_samples = VK_SAMPLE_COUNT_2_BIT;
        
        std::cout << "\t\tMax available MSAA samples: " << this->msaa_samples << std::endl;

    }
    else
    {
        std::cerr << "Failed to find suitable GPU!" << std::endl;
        return -1;
    }

    return 0;
}

std::int32_t vulkan_context_t::create_surface()
{
    if (glfwCreateWindowSurface(this->instance, this->window, nullptr, &(this->surface)) != VK_SUCCESS)
    {
        std::cerr << "Failed to create window surface!" << std::endl;
        return -1;
    }
    return 0;
}

std::optional<VkFramebuffer> vulkan_context_t::add_framebuffer(VkRenderPass render_pass, std::vector<VkImageView> attachments)
{
    VkFramebuffer framebuffer;
    VkFramebufferCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    create_info.renderPass = render_pass;
    create_info.attachmentCount = static_cast<std::uint32_t>(attachments.size());
    create_info.pAttachments = attachments.data();
    create_info.width = this->swap_chain->extent.width;
    create_info.height = this->swap_chain->extent.height;
    create_info.layers = 1;

    if (vkCreateFramebuffer(this->device->device, &create_info, nullptr, &framebuffer) != VK_SUCCESS)
    {
        std::cerr << "Failed to create framebuffer!" << std::endl;
        return std::nullopt;
    }
    return framebuffer;
}

std::int32_t vulkan_context_t::create_command_pool()
{
    VkCommandPoolCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    create_info.queueFamilyIndex = this->device->indices.graphics_family.value();
    
    if (vkCreateCommandPool(this->device->device, &create_info, nullptr, &(this->command_pool)) != VK_SUCCESS)
    {
        std::cerr << "Failed to create command pool!" << std::endl;
        return -1;
    }

    return 0;
}

std::int32_t vulkan_context_t::create_sync_objects()
{
    this->sync_objects.image_available.resize(MAX_FRAMES_IN_FLIGHT);
    this->sync_objects.render_finished.resize(MAX_FRAMES_IN_FLIGHT);
    this->sync_objects.in_flight.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (std::uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        if (vkCreateSemaphore(this->device->device, &semaphore_info, nullptr, &this->sync_objects.image_available[i]) != VK_SUCCESS
                || vkCreateSemaphore(this->device->device, &semaphore_info, nullptr, &this->sync_objects.render_finished[i]) != VK_SUCCESS
                || vkCreateFence(this->device->device, &fence_info, nullptr, &this->sync_objects.in_flight[i]) != VK_SUCCESS)
        {
            std::cerr << "Failed to create sync objects!" << std::endl;
            return -1;
        }
    }

    return 0;
}

std::int32_t vulkan_context_t::recreate_swap_chain()
{
    std::int32_t width = 0, height = 0;
    glfwGetFramebufferSize(this->window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(this->window, &width, &height);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(this->device->device);

    std::vector<image_settings_t> color_settings;
    for (image_t* img : this->color_buffers)
    {
        color_settings.push_back(img->settings);
        delete img;
    }
    std::vector<image_settings_t> depth_settings;
    for (image_t* img : this->depth_buffers)
    {
        depth_settings.push_back(img->settings);
        delete img;
    }

    delete this->swap_chain;

    this->swap_chain = new swap_chain_t(this->physical_device, this->surface);
    if (this->swap_chain->init(this->device, this->surface, this->window) != 0) return -1;
   
    std::uint32_t idx = 0;
    for (image_settings_t settings : color_settings)
    {
        this->color_buffers[idx] = new image_t(&this->physical_device, &this->command_pool);
        this->color_buffers[idx]->init_color_buffer(settings, this->get_swap_chain_extent(), this->device);
        this->color_buffers[idx]->transition_image_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        ++idx;
    }
    idx = 0;
    for (image_settings_t settings : depth_settings)
    {
        this->depth_buffers[idx] = new image_t(&this->physical_device, &this->command_pool);
        this->depth_buffers[idx]->init_depth_buffer(settings, this->get_swap_chain_extent(), this->device);
        ++idx;
    }

    for (render_pass_t* render_pass : this->render_passes)
    {
        std::uint32_t idx = 0;
        for (framebuffer_t fb : render_pass->framebuffers)
        {
            render_pass->recreate_framebuffer(idx, this->swap_chain->extent.width, this->swap_chain->extent.height);
            ++idx; 
        }
    }

    for (descriptor_pool_t* pool : this->descriptor_pools)
    {
        pool->reconfigure();
    }

    return 0;
}

std::int32_t vulkan_context_t::add_pipeline(const pipeline_shaders_t& shaders, const pipeline_settings_t& settings)
{
    graphics_pipeline_t* pipeline = new graphics_pipeline_t(settings.render_pass, this->swap_chain->extent);
    if (pipeline->init(shaders, settings, this->device) != 0) return -1;
    this->graphics_pipelines.push_back(pipeline);
   
    delete this->command_buffers;
    this->command_buffers = new command_buffers_t();
    if (this->command_buffers->init(this->command_pool, &(this->device->device), static_cast<std::uint32_t>(this->graphics_pipelines.size())) != 0) return -1;
    return 0;
}

std::int32_t vulkan_context_t::add_buffer(const buffer_settings_t& settings)
{
    buffer_t* buffer = new buffer_t(&this->physical_device, &this->command_pool);
    if (buffer->init(settings, this->device) != 0) return -1;
    this->buffers.push_back(buffer);
    return 0;
}

std::int32_t vulkan_context_t::add_image(const std::string& path, const image_settings_t& settings, bool flip)
{
    image_t* image = new image_t(&this->physical_device, &this->command_pool);
    if (image->init_texture(path, settings, this->device, flip) != 0) return -1;
    this->images.push_back(image);
    return 0;
}

std::int32_t vulkan_context_t::add_descriptor_set_layout(const std::vector<VkDescriptorSetLayoutBinding> layout_bindings)
{
    VkDescriptorSetLayout set_layout;
    VkDescriptorSetLayoutCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.bindingCount = static_cast<std::uint32_t>(layout_bindings.size());
    create_info.pBindings = layout_bindings.data();
    std::vector<VkDescriptorType> types;
    std::for_each(layout_bindings.begin(), layout_bindings.end(), [&](VkDescriptorSetLayoutBinding e){ types.push_back(e.descriptorType); });

    if (vkCreateDescriptorSetLayout(this->device->device, &create_info, nullptr, &set_layout) != VK_SUCCESS)
    {
        std::cerr << "Failed to create descriptor set layout" << std::endl;
        return -1;
    }

    this->descriptor_set_layouts.push_back(set_layout);
    
    descriptor_pool_t* descriptor_pool = new descriptor_pool_t();
    if (descriptor_pool->init(this->descriptor_set_layouts.back(), types, &this->device->device) != 0) return -1;
    this->descriptor_pools.push_back(descriptor_pool);
    
    return 0;
}

void vulkan_context_t::bind_descriptor_sets(VkCommandBuffer command_buffer, std::uint32_t pool_index, std::uint32_t first_set)
{
    pool_index = 0;
    for (graphics_pipeline_t* pipe : this->graphics_pipelines)
    {
        if (pipe == this->current_pipeline) break;
        pool_index++;
    }
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->current_pipeline->pipeline_layout, first_set, 1,
            &this->descriptor_pools[pool_index]->sets[this->current_frame], 0, nullptr);
}

std::int32_t vulkan_context_t::draw_frame(std::function<void(VkCommandBuffer, vulkan_context_t*)> func)
{
    vkWaitForFences(this->device->device, 1, &this->sync_objects.in_flight[this->current_frame], VK_TRUE, UINT64_MAX);

    std::uint32_t image_index;
    VkResult result = vkAcquireNextImageKHR(this->device->device, this->swap_chain->swap_chain, UINT64_MAX,
            this->sync_objects.image_available[this->current_frame], VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        if (recreate_swap_chain() != 0)
        {
            return -1;
        }
        return 0;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        std::cerr << "Failed to aquire swap chain image!" << std::endl;
        return -1;
    }

    vkResetFences(this->device->device, 1, &this->sync_objects.in_flight[this->current_frame]);
    vkResetCommandBuffer(this->command_buffers->command_buffers[this->current_frame], 0);

    std::vector<VkCommandBuffer> command_buffers;
    
    recording_settings_t settings{};
    settings.populate_defaults(this->render_passes[0]->render_pass, this->render_passes[0]->framebuffers[image_index].framebuffer,
            this->swap_chain->extent, G_CLEAR_COLORS);
    settings.draw_command = [&] (VkCommandBuffer command_buffer) { func(command_buffer, this); };

    if (this->command_buffers->record(this->current_frame, settings) != 0)
    {
        return -1;
    }
    command_buffers.push_back(this->command_buffers->command_buffers[this->current_frame]);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = { this->sync_objects.image_available[this->current_frame] };
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;

    submit_info.commandBufferCount = static_cast<std::uint32_t>(command_buffers.size());
    submit_info.pCommandBuffers = command_buffers.data();

    VkSemaphore signal_semaphores[] = { this->sync_objects.render_finished[this->current_frame] };
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    if (vkQueueSubmit(this->device->graphics_queue, 1, &submit_info, this->sync_objects.in_flight[this->current_frame]) != VK_SUCCESS)
    {
        std::cerr << "Failed to submit draw command buffer to queue!" << std::endl;
        return -1;
    }

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swap_chains[] = { this->swap_chain->swap_chain };
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains;
    present_info.pImageIndices = &image_index;

    result = vkQueuePresentKHR(this->device->present_queue, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || this->framebuffer_resized)
    {
        this->framebuffer_resized = false;
        if (recreate_swap_chain() != 0)
        {
            return -1;
        }
    }
    else if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to present swap chain image!" << std::endl;
        return -1;
    }

    this->current_frame = (this->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

    return 0;
}

void vulkan_context_t::main_loop(std::function<void()> func)
{
    func();
    vkDeviceWaitIdle(this->device->device);
}

vulkan_context_t::vulkan_context_t(std::string name, std::uint32_t width, std::uint32_t height)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    this->window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(this->window, this);
    glfwSetFramebufferSizeCallback(this->window, framebuffer_resize_callback);

    if (create_instance(name) != 0) return;
    if (ENABLE_VALIDATION_LAYERS)
    {
        this->debug_messenger = new debug_messenger_t();
        if (this->debug_messenger->init(&(this->instance), nullptr) != VK_SUCCESS)
        {
            std::cerr << "Failed to set up debug messenger!" << std::endl;
            return;
        }
    }
    if (create_surface() != 0) return;
    if (pick_physical_device() != 0) return;
    
    this->device = new logical_device_t(&(this->physical_device), this->surface);
    if (this->device->init() != 0) return;
    
    this->swap_chain = new swap_chain_t(this->physical_device, this->surface);
    if (this->swap_chain->init(this->device, this->surface, this->window) != 0) return;

    if (create_command_pool() != 0) return;
    if (create_sync_objects() != 0) return;

    this->initialized = true;
}

vulkan_context_t::~vulkan_context_t()
{

    for (buffer_t* buf : this->buffers)
    {
        delete buf;
    }

    for (image_t* img : this->images)
    {
        delete img;
    }

    for (image_t* img : this->color_buffers)
        delete img;

    for (image_t* img : this->depth_buffers)
        delete img;

    for (descriptor_pool_t* pool : this->descriptor_pools)
    {
        delete pool;
    }

    for (VkDescriptorSetLayout layout : this->descriptor_set_layouts)
    {
        vkDestroyDescriptorSetLayout(this->device->device, layout, nullptr);
    }
    std::cout << "Destroying Descriptor Set Layouts!" << std::endl;

    for (std::uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroySemaphore(this->device->device, this->sync_objects.image_available[i], nullptr);
        vkDestroySemaphore(this->device->device, this->sync_objects.render_finished[i], nullptr);
        vkDestroyFence(this->device->device, this->sync_objects.in_flight[i], nullptr);
    }
    std::cout << "Destroying Sync Objects!" << std::endl;
    
    vkDestroyCommandPool(this->device->device, this->command_pool, nullptr);
    std::cout << "Destroying Command Pool!" << std::endl;
    
    for (graphics_pipeline_t* pipeline : this->graphics_pipelines)
    {
        delete pipeline;
    }
    
    for (render_pass_t* render_pass : this->render_passes)
    {
        delete render_pass;
    }
    
    delete this->swap_chain;
    delete this->device;

    if (ENABLE_VALIDATION_LAYERS)
    {
        delete this->debug_messenger;
    }

    vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
    std::cout << "Destroying Surface!" << std::endl;
    vkDestroyInstance(this->instance, nullptr);
    std::cout << "Destroying Instance!" << std::endl;
    glfwDestroyWindow(this->window);
    std::cout << "Destroying Vulkan Context!" << std::endl;
}

VkExtent2D vulkan_context_t::get_swap_chain_extent()
{
    return this->swap_chain->extent;
}

std::vector<VkDescriptorSetLayout> vulkan_context_t::get_descriptor_set_layouts()
{
    return this->descriptor_set_layouts;
}

std::uint32_t vulkan_context_t::get_current_frame()
{
    return this->current_frame;
}

buffer_t* vulkan_context_t::get_buffer(std::uint32_t index)
{
    return this->buffers[index];
}

buffer_t* vulkan_context_t::get_last_buffer()
{
    return this->buffers.back();
}

std::uint32_t vulkan_context_t::get_buffer_count()
{
    return this->buffers.size();
}

image_t* vulkan_context_t::get_image(std::uint32_t index)
{
    return this->images[index];
}

std::vector<descriptor_pool_t*>* vulkan_context_t::get_descriptor_pools()
{
    return &this->descriptor_pools;
}

void vulkan_context_t::framebuffer_resize_callback(GLFWwindow* window, std::int32_t width, std::int32_t height)
{
    vulkan_context_t* context = reinterpret_cast<vulkan_context_t*>(glfwGetWindowUserPointer(window));
    context->framebuffer_resized = true;
}
