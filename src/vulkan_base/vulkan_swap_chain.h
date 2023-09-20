#pragma once

#include "vulkan_logical_device.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <cstdint>
#include <vulkan/vulkan_core.h>

class swap_chain_t
{
    private:
        const VkDevice* device = nullptr;
        const VkAllocationCallbacks* allocator = nullptr;

        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> present_modes;
        
        void choose_format();
        void choose_present_mode();
        void choose_extent(GLFWwindow* window);

    public:
        VkSwapchainKHR swap_chain;
        std::vector<VkImage> swap_chain_images;
        VkSurfaceFormatKHR format;
        VkPresentModeKHR present_mode;
        VkExtent2D extent;

        bool is_adequate();
        std::int32_t init(const logical_device_t* logical_device, VkSurfaceKHR& surface, GLFWwindow* window);
        swap_chain_t(const VkPhysicalDevice& physical_device, const VkSurfaceKHR& surface);
        ~swap_chain_t();
};
