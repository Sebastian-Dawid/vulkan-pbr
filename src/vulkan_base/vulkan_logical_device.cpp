#include "vulkan_logical_device.h"
#include "vulkan_constants.h"
#include "vulkan_validation_layers.h"
#include <iostream>
#include <set>

#include "debug_print.h"

std::int32_t logical_device_t::init()
{
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<std::uint32_t> unique_queue_families = { indices.graphics_family.value(), indices.present_family.value() };

    float queue_priority = 1.0f;
    for (std::uint32_t queue_family : unique_queue_families)
    {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures device_features{};
    device_features.samplerAnisotropy = VK_TRUE;
    device_features.sampleRateShading = VK_TRUE;

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.queueCreateInfoCount = static_cast<std::uint32_t>(queue_create_infos.size());
    create_info.pEnabledFeatures = &device_features;

    create_info.enabledExtensionCount = static_cast<std::uint32_t>(device_extensions.size());
    create_info.ppEnabledExtensionNames = device_extensions.data();

    if (ENABLE_VALIDATION_LAYERS)
    {
        create_info.enabledLayerCount = static_cast<std::uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();
    }
    else
    {
        create_info.enabledLayerCount = 0;
    }
    if (vkCreateDevice(*(physical_device), &create_info, this->allocator, &(this->device)) != VK_SUCCESS)
    {
        std::cerr << "Failed to create logical device!" << std::endl;
        return -1;
    }
    
    vkGetDeviceQueue(this->device, this->indices.graphics_family.value(), 0, &(this->graphics_queue));
    vkGetDeviceQueue(this->device, this->indices.present_family.value(), 0, &(this->present_queue));

    return 0;
}

logical_device_t::logical_device_t(VkPhysicalDevice* physical_device, VkSurfaceKHR& surface) : indices(*physical_device, surface)
{
    this->physical_device = physical_device;
}

logical_device_t::~logical_device_t()
{
    vkDestroyDevice(this->device, this->allocator);
    DEBUG_PRINT("Destroying Logical Device!");
}
