#pragma once

#include <cstdint>
#include <optional>
#include <vulkan/vulkan_core.h>

class queue_family_indices_t
{
    public:
        std::optional<std::uint32_t> graphics_family;
        std::optional<std::uint32_t> present_family;
        bool is_complete();
        queue_family_indices_t(VkPhysicalDevice physical_device, VkSurfaceKHR& surface);
};
