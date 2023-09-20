#include "vulkan_base.h"
#include "vulkan_constants.h"
#include "vulkan_queue_family_indices.h"
#include "vulkan_validation_layers.h"
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
    glfw_exts.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

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
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    std::vector<const char*> glfw_exts = get_required_extensions();

    create_info.enabledExtensionCount = glfw_exts.size();
    create_info.ppEnabledExtensionNames = glfw_exts.data();
    create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

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

    if (!device_features.geometryShader)
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

vulkan_context_t::vulkan_context_t(std::string name)
{
    this->window = glfwCreateWindow(1280, 720, name.c_str(), nullptr, nullptr);
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
    if (this->device->init() != 0)
    {
        return;
    }
    this->swap_chain = new swap_chain_t(this->physical_device, this->surface);
    if (this->swap_chain->init(this->device, this->surface, this->window) != 0)
    {
        return;
    }

    this->initialized = true;
}

vulkan_context_t::~vulkan_context_t()
{
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
