#include "vulkan_swap_chain.h"
#include <algorithm>
#include <iostream>
#include <limits>

void swap_chain_t::choose_format()
{
    for (const VkSurfaceFormatKHR& fmt : this->formats)
    {
        if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            this->format = fmt;
            return;
        }
    }
    this->format = this->formats[0];
}

void swap_chain_t::choose_present_mode()
{
    for (const VkPresentModeKHR& pm : this->present_modes)
    {
        if (pm == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            this->present_mode = pm;
            return;
        }
    }
    this->present_mode = VK_PRESENT_MODE_FIFO_KHR;
}

void swap_chain_t::choose_extent(GLFWwindow* window)
{
    if (this->capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max())
    {
        this->extent = capabilities.currentExtent;
        return;
    }
    
    std::int32_t width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actual_extent = {
        static_cast<std::uint32_t>(width),
        static_cast<std::uint32_t>(height)
    };
    actual_extent.width = std::clamp(actual_extent.width, this->capabilities.minImageExtent.width, this->capabilities.maxImageExtent.width);
    actual_extent.height = std::clamp(actual_extent.height, this->capabilities.minImageExtent.height, this->capabilities.maxImageExtent.height);
    this->extent = actual_extent;
}

bool swap_chain_t::is_adequate()
{
    return !this->formats.empty() && !this->present_modes.empty();
}

std::int32_t swap_chain_t::init(const logical_device_t* logical_device, VkSurfaceKHR& surface, GLFWwindow* window)
{
    choose_format();
    choose_present_mode();
    choose_extent(window);

    std::uint32_t image_count = this->capabilities.minImageCount + 1;
    if (this->capabilities.maxImageCount > 0 && image_count > this->capabilities.maxImageCount)
    {
        image_count = this->capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface;

    create_info.minImageCount = image_count;
    create_info.imageFormat = this->format.format;
    create_info.imageColorSpace = this->format.colorSpace;
    create_info.imageExtent = this->extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (logical_device->indices.graphics_family.value() != logical_device->indices.present_family.value())
    {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        std::uint32_t queue_family_indices[] = { logical_device->indices.graphics_family.value(), logical_device->indices.present_family.value() };
        create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else
    {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    
    create_info.preTransform = this->capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = this->present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(logical_device->device, &create_info, this->allocator, &(this->swap_chain)) != VK_SUCCESS)
    {
        std::cerr << "Failed to create swap chain!" << std::endl;
        return -1;
    }

    this->device = &(logical_device->device);
  
    vkGetSwapchainImagesKHR(logical_device->device, this->swap_chain, &image_count, nullptr);
    this->images.resize(image_count);
    vkGetSwapchainImagesKHR(logical_device->device, this->swap_chain, &image_count, this->images.data());
    
    this->image_views.resize(image_count);
    for (std::uint32_t i = 0; i < image_count; ++i)
    {
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = this->images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = this->format.format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(logical_device->device, &create_info, nullptr, &(this->image_views[i])) != VK_SUCCESS)
        {
            std::cerr << "Failed to create image views!" << std::endl;
            return -1;
        }

    }

    return 0;
}

swap_chain_t::swap_chain_t(const VkPhysicalDevice& physical_device, const VkSurfaceKHR& surface)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &(this->capabilities));
    
    std::uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
    if (format_count != 0)
    {
        this->formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, this->formats.data());
    }

    std::uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);
    if (present_mode_count != 0)
    {
        this->present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, this->present_modes.data());
    }
}

swap_chain_t::~swap_chain_t()
{
    if (this->device == nullptr)
    {
        return;
    }
    for (VkImageView image_view : this->image_views)
    {
        vkDestroyImageView(*(this->device), image_view, nullptr);
    }
    std::cout << "Destroying Image Views!" << std::endl;
    vkDestroySwapchainKHR(*(this->device), this->swap_chain, this->allocator);
    std::cout << "Destroying Swap Chain!" << std::endl;
}
