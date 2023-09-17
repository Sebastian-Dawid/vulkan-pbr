#pragma once

#include <vulkan/vulkan_core.h>

void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info);

class debug_messenger_t
{
    private:
        VkDebugUtilsMessengerCreateInfoEXT create_info;
        VkDebugUtilsMessengerEXT debug_messenger;
        VkInstance* instance;
        const VkAllocationCallbacks* allocator;
        bool initialized = false;
    public:
        debug_messenger_t();
        VkResult init(VkInstance* instance, const VkAllocationCallbacks* allocator);
        ~debug_messenger_t();
};
