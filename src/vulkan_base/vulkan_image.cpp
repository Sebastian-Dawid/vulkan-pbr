#include "vulkan_image.h"
#include "vulkan_buffer.h"
#include "vulkan_command_buffer.h"
#include <iostream>

std::int32_t create_image_view(VkImageView &view, VkImage image, VkFormat format, VkDevice device)
{
    VkImageViewCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = image;
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = format;
    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &create_info, nullptr, &view) != VK_SUCCESS)
    {
        std::cerr << "Failed to create image view!" << std::endl;
        return -1;
    }

    return 0;
}

std::int32_t image_t::create_image(std::uint32_t width, std::uint32_t height, VkImage& image, VkDeviceMemory& memory)
{
    VkImageCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.imageType = this->settings->type;
    create_info.extent.width = width;
    create_info.extent.height = height;
    create_info.extent.depth = 1;
    create_info.mipLevels = this->settings->mip_levels;
    create_info.arrayLayers = 1;
    create_info.format = this->settings->format;
    create_info.tiling = this->settings->tiling;
    create_info.initialLayout = this->settings->layout;
    create_info.usage = this->settings->usage;
    create_info.samples = this->settings->sample_count;
    create_info.sharingMode = this->settings->sharing_mode;

    if (vkCreateImage(this->device->device, &create_info, this->allocator, &image) != VK_SUCCESS)
    {
        std::cerr << "Failed to create image!" << std::endl;
        return -1;
    }

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(this->device->device, image, &mem_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    std::optional<std::uint32_t> idx = find_memory_type(mem_requirements.memoryTypeBits, *this->physical_device, this->settings->properties);
    if (!idx.has_value())
    {
        std::cerr << "Failed to find suitable memory type!" << std::endl;
        return -1;
    }
    alloc_info.memoryTypeIndex = idx.value();

    if (vkAllocateMemory(this->device->device, &alloc_info, nullptr, &memory) != VK_SUCCESS)
    {
        std::cerr << "Failed to allocate image memory!" << std::endl;
        return -1;
    }

    vkBindImageMemory(this->device->device, image, memory, 0);

    return 0;
}

std::int32_t image_t::create_image_sampler(const sampler_settings_t& settings)
{
    VkSamplerCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    create_info.magFilter = settings.filter.mag;
    create_info.minFilter = settings.filter.min;
    create_info.addressModeU = settings.address_mode.u;
    create_info.addressModeV = settings.address_mode.v;
    create_info.addressModeW = settings.address_mode.w;
    create_info.anisotropyEnable = settings.anisotropy_enable;
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(*this->physical_device, &properties);
    create_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    create_info.borderColor = settings.border_color;
    create_info.unnormalizedCoordinates = settings.unnormalized_coordinates;
    create_info.compareEnable = settings.compare_enable;
    create_info.compareOp = settings.compare_op;
    create_info.mipmapMode = settings.mipmap_mode;
    create_info.mipLodBias = settings.lod.bias;
    create_info.maxLod = settings.lod.max;
    create_info.minLod = settings.lod.min;
    if (vkCreateSampler(this->device->device, &create_info, nullptr, &this->sampler) != VK_SUCCESS)
    {
        std::cerr << "Failed to create sampler!" << std::endl;
        return -1;
    }
    return 0;
}

std::int32_t image_t::transition_image_layout(VkImageLayout layout)
{
    VkCommandBuffer command_buffer = begin_single_time_commands(this->device->device, *this->command_pool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = this->layout;
    barrier.newLayout = layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = this->image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags src_stage, dst_stage;

    if (this->layout == VK_IMAGE_LAYOUT_UNDEFINED && layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (this->layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        std::cerr << "Unsupported layout transition!" << std::endl;
        return -1;
    }

    vkCmdPipelineBarrier(
        command_buffer,
        src_stage, dst_stage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    this->layout = layout;

    end_single_time_commands(*this->command_pool, command_buffer, this->device->device, this->device->graphics_queue);
    return 0;
}

void image_t::copy_buffer_to_image(VkBuffer buffer)
{
    VkCommandBuffer command_buffer = begin_single_time_commands(this->device->device, *this->command_pool);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        this->width,
        this->height,
        1
    };

    vkCmdCopyBufferToImage(command_buffer, buffer, this->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    end_single_time_commands(*this->command_pool, command_buffer, this->device->device, this->device->graphics_queue);
}

std::int32_t image_t::init_texture(const std::string& path, const image_settings_t& settings, const logical_device_t* device)
{
    std::int32_t width, height, channels;
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    VkDeviceSize image_size = width * height * 4;
    if (!pixels)
    {
        std::cerr << "Failed to load image: " << path << "!" << std::endl;
        return -1;
    }

    buffer_t staging_buffer(this->physical_device, this->command_pool);
    buffer_settings_t buffer_settings;
    buffer_settings.size = image_size;
    buffer_settings.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_settings.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    staging_buffer.init(buffer_settings, device);
    staging_buffer.set_data(pixels);
    
    this->settings = &settings;
    this->device = device;
    if (create_image(width, height, this->image, this->memory) != 0)
    {
        this->device = nullptr;
        return -1;
    }

    this->width = width;
    this->height = height;
    this->format = settings.format;
    this->layout = settings.layout;

    transition_image_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copy_buffer_to_image(staging_buffer.buffer);
    transition_image_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    create_image_view(this->view, this->image, settings.format, device->device);
    sampler_settings_t sampler_settings;
    create_image_sampler(sampler_settings);

    return 0;
}

image_t::image_t(const VkPhysicalDevice* physical_device, const VkCommandPool* command_pool)
{
    this->physical_device = physical_device;
    this->command_pool = command_pool;
}

image_t::~image_t()
{
    if (this->device == nullptr) return;
    vkDestroySampler(this->device->device, this->sampler, nullptr);
    std::cout << "Destroying Image Sampler!" << std::endl;
    vkDestroyImageView(this->device->device, this->view, nullptr);
    std::cout << "Destroying Image View!" << std::endl;
    vkDestroyImage(this->device->device, this->image, this->allocator);
    std::cout << "Destroying Image!" << std::endl;
    vkFreeMemory(this->device->device, this->memory, nullptr);
    std::cout << "Freeing Image Memory!" << std::endl;
}
