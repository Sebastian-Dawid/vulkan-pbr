#pragma once

#include "vulkan_logical_device.h"
#include <stb_image.h>
#include <string>
#include <vector>

struct image_settings_t
{
    VkImageType type = VK_IMAGE_TYPE_2D;
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    std::uint32_t mip_levels = 1;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT;
    VkSharingMode sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
};

struct sampler_settings_t
{
    struct
    {
        VkFilter mag = VK_FILTER_LINEAR, min = VK_FILTER_LINEAR;
    } filter;
    struct
    {
        VkSamplerAddressMode u = VK_SAMPLER_ADDRESS_MODE_REPEAT, v = VK_SAMPLER_ADDRESS_MODE_REPEAT, w = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    } address_mode;
    VkBool32 anisotropy_enable = VK_TRUE;
    VkBorderColor border_color = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    VkBool32 unnormalized_coordinates = VK_FALSE;
    VkBool32 compare_enable = VK_FALSE;
    VkCompareOp compare_op = VK_COMPARE_OP_ALWAYS;
    VkSamplerMipmapMode mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    struct
    {
        float bias = 0.0f, min = 0.0f, max = 0.0f;
    } lod;
};

class image_t
{
    private:
        const VkPhysicalDevice* physical_device = nullptr;
        const VkCommandPool* command_pool = nullptr;
        const logical_device_t* device = nullptr;
        const VkAllocationCallbacks* allocator = nullptr;
        image_settings_t settings{};

        std::int32_t create_image(std::uint32_t width, std::uint32_t height, VkImage& image, VkDeviceMemory& memory);
        std::int32_t create_image_sampler(const sampler_settings_t& settings);
        std::int32_t transition_image_layout(VkImageLayout layout);
        void copy_buffer_to_image(VkBuffer buffer);
        std::int32_t generate_mipmaps();

    public:
        std::uint32_t width;
        std::uint32_t height;
        VkImage image;
        VkImageView view;
        VkSampler sampler;
        VkDeviceMemory memory;
        VkImageLayout layout;
        VkFormat format;

        std::int32_t init_texture(const std::string& path, const image_settings_t& settings, const logical_device_t* device, bool flip = false);
        std::int32_t init_depth_buffer(image_settings_t settings, const VkExtent2D& extent, const logical_device_t* device);
        std::int32_t init_color_buffer(image_settings_t settings, const VkExtent2D& extent, const logical_device_t* device);
        image_t(const VkPhysicalDevice* physical_device, const VkCommandPool* command_pool);
        ~image_t();
};

std::optional<VkFormat> find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags featrues, const VkPhysicalDevice* physical_device);
std::optional<VkFormat> find_depth_format(const VkPhysicalDevice* physical_device);
std::int32_t create_image_view(VkImageView& view, VkImage image, VkFormat format, VkDevice device, VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT, std::uint32_t mip_levels = 1);
