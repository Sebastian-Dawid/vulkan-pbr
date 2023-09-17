#include "vulkan_queue_family_indices.h"
#include <iostream>
#include <vector>

bool queue_family_indices_t::is_complete()
{
    return this->graphics_family.has_value();
}

queue_family_indices_t::queue_family_indices_t(VkPhysicalDevice physical_device)
{
    std::uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

    std::cout << "\t\tQueues: " << queue_family_count << std::endl;

    std::uint32_t i = 0;
    for (const VkQueueFamilyProperties& queue_family : queue_families)
    {
        if (this->is_complete()) break;
        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            this->graphics_family = i;
        }
        ++i;
    }
}
