#include "vulkan_base.h"
#include <cstring>
#include <iostream>

std::int32_t vulkan_context_t::create_instance(std::string name)
{
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

    std::uint32_t glfw_ext_count = 0;
    const char** glfw_exts;
    glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

    create_info.enabledExtensionCount = glfw_ext_count;
    create_info.ppEnabledExtensionNames = glfw_exts;

    create_info.enabledLayerCount = 0;
    
    std::uint32_t ext_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr);
    std::vector<VkExtensionProperties> exts(ext_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, exts.data());

    std::function<bool()> check_for_required_exts = [&]
    {
        std::uint32_t count = 0;
        for (std::uint32_t i = 0; i < glfw_ext_count; ++i)
        {
            for (const VkExtensionProperties& ext : exts)
            {
                if (strcmp(glfw_exts[i], ext.extensionName) == 0) 
                {
                    count++;
                    break;
                }
            }
        }
        return count == glfw_ext_count;
    };

    if (!check_for_required_exts())
    {
        std::cerr << "Required extention(s) not available!" << std::endl;
        return -1;
    }

    std::cout << "Available Extentions:" << std::endl;
    for (const VkExtensionProperties& ext : exts)
    {
        std::cout << "\t" << ext.extensionName << std::endl;
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
    create_instance(name);
}

vulkan_context_t::~vulkan_context_t()
{
    vkDestroyInstance(this->instance, nullptr);
    glfwDestroyWindow(this->window);
    glfwTerminate();
}
