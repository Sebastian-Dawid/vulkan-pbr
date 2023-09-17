#include "vulkan_base.h"
#include "vulkan_validation_layers.h"
#include <cstring>
#include <iostream>

std::vector<const char*> get_required_extentions()
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

    std::vector<const char*> glfw_exts = get_required_extentions();

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

vulkan_context_t::vulkan_context_t(std::string name)
{
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
    this->initialized = true;
}

vulkan_context_t::~vulkan_context_t()
{
    std::cout << "Destroying Vulkan Context!" << std::endl;
    if (ENABLE_VALIDATION_LAYERS)
    {
        delete this->debug_messenger;
    }

    vkDestroyInstance(this->instance, nullptr);
    glfwDestroyWindow(this->window);
}
