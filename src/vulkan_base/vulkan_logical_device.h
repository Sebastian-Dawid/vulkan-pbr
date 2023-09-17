#pragma once

#include "vulkan_queue_family_indices.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

class logical_device_t
{
    private:
        VkDevice device;
        VkDeviceCreateInfo create_info{};

        queue_family_indices_t indices;
        VkQueue graphics_queue;
        VkDeviceQueueCreateInfo queue_create_info{};

        const VkPhysicalDevice* physical_device;
        const VkAllocationCallbacks* allocator = nullptr;
    public:
        std::int32_t init();
        logical_device_t(VkPhysicalDevice* physical_device);
        ~logical_device_t();
};
