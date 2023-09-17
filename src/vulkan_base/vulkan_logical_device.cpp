#include "vulkan_logical_device.h"
#include "vulkan_validation_layers.h"
#include <iostream>

std::int32_t logical_device_t::init()
{
    this->queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    this->queue_create_info.queueFamilyIndex = this->indices.graphics_family.value();
    this->queue_create_info.queueCount = 1;

    float queue_priority = 1.0f;
    this->queue_create_info.pQueuePriorities = &queue_priority;

    VkPhysicalDeviceFeatures device_features{};

    this->create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    this->create_info.pQueueCreateInfos = &this->queue_create_info;
    this->create_info.queueCreateInfoCount = 1;
    this->create_info.pEnabledFeatures = &device_features;

    this->create_info.enabledExtensionCount = 0;

    if (ENABLE_VALIDATION_LAYERS)
    {
        this->create_info.enabledLayerCount = static_cast<std::uint32_t>(validation_layers.size());
        this->create_info.ppEnabledLayerNames = validation_layers.data();
    }
    else
    {
        this->create_info.enabledLayerCount = 0;
    }
    if (vkCreateDevice(*(this->physical_device), &(this->create_info), this->allocator, &(this->device)) != VK_SUCCESS)
    {
        std::cerr << "Failed to create logical device!" << std::endl;
        return -1;
    }
    
    vkGetDeviceQueue(this->device, this->indices.graphics_family.value(), 0, &(this->graphics_queue));

    return 0;
}

logical_device_t::logical_device_t(VkPhysicalDevice* physical_device) : indices(*physical_device)
{
    this->physical_device = physical_device;
}

logical_device_t::~logical_device_t()
{
    vkDestroyDevice(this->device, this->allocator);
    std::cout << "Destroying Logical Device!" << std::endl;
}
