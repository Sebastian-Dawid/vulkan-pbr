#pragma once

#include "vulkan_queue_family_indices.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

class logical_device_t
{
    private:
        const VkPhysicalDevice* physical_device;
        const VkAllocationCallbacks* allocator = nullptr;
    
    public:
        VkDevice device;
        VkQueue graphics_queue;
        VkQueue present_queue;
        queue_family_indices_t indices;
        
        std::int32_t init();
        logical_device_t(VkPhysicalDevice* physical_device, VkSurfaceKHR& surface);
        ~logical_device_t();
};
