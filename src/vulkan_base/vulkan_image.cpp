#include "vulkan_image.h"
#include "vulkan_buffer.h"
#include "vulkan_command_buffer.h"
#include <cmath>
#include <iostream>

bool has_stencil_component(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

std::int32_t create_image_view(image_view_settings_t& settings)
{
    VkImageViewCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = settings.image;
    create_info.viewType = settings.type;
    create_info.format = settings.format;
    create_info.components = settings.components;
    create_info.subresourceRange.aspectMask = settings.aspect_mask;
    create_info.subresourceRange.baseMipLevel = settings.base_mip_level;
    create_info.subresourceRange.levelCount = settings.mip_levels;
    create_info.subresourceRange.baseArrayLayer = settings.base_array_layer;
    create_info.subresourceRange.layerCount = settings.layer_count;

    if (vkCreateImageView(settings.device, &create_info, nullptr, settings.view) != VK_SUCCESS)
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
    create_info.imageType = this->settings.type;
    create_info.extent.width = width;
    create_info.extent.height = height;
    create_info.extent.depth = 1;
    create_info.mipLevels = this->settings.mip_levels;
    create_info.arrayLayers = this->settings.layer_count;
    create_info.format = this->settings.format;
    create_info.tiling = this->settings.tiling;
    create_info.initialLayout = this->settings.layout;
    create_info.usage = this->settings.usage;
    create_info.samples = this->settings.sample_count;
    create_info.sharingMode = this->settings.sharing_mode;
    create_info.flags = this->settings.flags;

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
    std::optional<std::uint32_t> idx = find_memory_type(mem_requirements.memoryTypeBits, *this->physical_device, this->settings.properties);
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
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = this->settings.mip_levels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = this->settings.layer_count;

    VkPipelineStageFlags src_stage, dst_stage;

    if (layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (has_stencil_component(this->format))
        {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

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
    else if (this->layout == VK_IMAGE_LAYOUT_UNDEFINED && layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (this->layout == VK_IMAGE_LAYOUT_UNDEFINED && layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
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

std::optional<VkFormat> find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, const VkPhysicalDevice* physical_device)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(*physical_device, format, &props);
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    return std::nullopt;
}

std::optional<VkFormat> find_depth_format(const VkPhysicalDevice* physical_device)
{
    return find_supported_format(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
            physical_device
            );
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

std::int32_t image_t::generate_mipmaps()
{
    
    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(*this->physical_device, this->format, &format_properties);

    if (!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {
        std::cerr << "Image format does not support linear blitting!" << std::endl;
        return -1;
    }

    VkCommandBuffer command_buffer = begin_single_time_commands(this->device->device, *this->command_pool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = this->image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    std::int32_t mip_width = this->width;
    std::int32_t mip_height = this->height;

    for (std::uint32_t i = 1; i < this->settings.mip_levels; ++i)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(command_buffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mip_width, mip_height, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(command_buffer,
                this->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                this->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        vkCmdPipelineBarrier(command_buffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

        if (mip_width > 1) mip_width /= 2;
        if (mip_height > 1) mip_height /= 2;
    }

    barrier.subresourceRange.baseMipLevel = this->settings.mip_levels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
    vkCmdPipelineBarrier(command_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

    this->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    end_single_time_commands(*this->command_pool, command_buffer, this->device->device, this->device->graphics_queue);

    return 0;
}

std::int32_t image_t::init_texture(const std::string& path, const image_settings_t& settings, const logical_device_t* device, bool flip)
{
    std::int32_t width, height, channels;
    stbi_set_flip_vertically_on_load(flip);
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    VkDeviceSize image_size = width * height * 4;
    if (!pixels)
    {
        std::cerr << "Failed to load image: " << path << "!" << std::endl;
        return -1;
    }

    this->settings = settings;
    this->settings.mip_levels = static_cast<std::uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    buffer_t staging_buffer(this->physical_device, this->command_pool);
    buffer_settings_t buffer_settings;
    buffer_settings.size = image_size;
    buffer_settings.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_settings.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    staging_buffer.init(buffer_settings, device);
    staging_buffer.set_data(pixels);
    
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
    generate_mipmaps();

    image_view_settings_t image_view_settings = {
        .view = &this->view,
        .image = this->image,
        .format = this->format,
        .device = this->device->device,
    };
    create_image_view(image_view_settings);
    sampler_settings_t sampler_settings;
    sampler_settings.lod.max = static_cast<float>(this->settings.mip_levels);
    create_image_sampler(sampler_settings);

    return 0;
}

std::int32_t image_t::init_depth_buffer(image_settings_t settings, const VkExtent2D& extent, const logical_device_t* device)
{
    std::optional<VkFormat> depth_format = find_depth_format(this->physical_device);
    if (!depth_format.has_value())
    {
        std::cerr << "Failed to find depth format!" << std::endl;
        return -1;
    }

    settings.format = depth_format.value();

    this->settings = settings;
    this->device = device;
    if (create_image(extent.width, extent.height, this->image, this->memory) != 0)
    {
        this->device = nullptr;
        return -1;
    }

    this->width = extent.width;
    this->height = extent.height;
    this->format = settings.format;
    this->layout = settings.layout;
    this->sampler = VK_NULL_HANDLE;

    image_view_settings_t image_view_settings = {
        .view = &this->view,
        .image = this->image,
        .format = settings.format,
        .device = device->device,
        .aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT
    };
    create_image_view(image_view_settings);
    transition_image_layout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    return 0;
}

std::int32_t image_t::init_color_buffer(image_settings_t settings, const VkExtent2D& extent, const logical_device_t* device, std::optional<image_view_settings_t> view_settings)
{
    this->settings = settings;
    this->device = device;
    if (create_image(extent.width, extent.height, this->image, this->memory) != 0)
    {
        this->device = nullptr;
        return -1;
    }

    this->width = extent.width;
    this->height = extent.height;
    this->format = settings.format;
    this->layout = settings.layout;
    this->sampler = VK_NULL_HANDLE;

    image_view_settings_t image_view_settings;
    if (view_settings.has_value())
    {
        image_view_settings = view_settings.value();
    }
    else
    {
        image_view_settings = {
            .format = settings.format,
            .mip_levels = settings.mip_levels,
            .layer_count = settings.layer_count
        };
    }
    image_view_settings.view = &this->view;
    image_view_settings.image = this->image;
    image_view_settings.device = device->device;
    create_image_view(image_view_settings);
    if (image_view_settings.layer_count == 1) return 0;

    image_view_settings.type = VK_IMAGE_VIEW_TYPE_2D;
    image_view_settings.layer_count = 1;
    image_view_settings.image = this->image;
    for (std::uint32_t i = 0; i < view_settings->layer_count; ++i)
    {
        image_view_settings.base_array_layer = i;
        this->secondary_views.push_back(nullptr);
        image_view_settings.view = &this->secondary_views[i];
        create_image_view(image_view_settings);
    }

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
    if (this->sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(this->device->device, this->sampler, nullptr);
        std::cout << "Destroying Image Sampler!" << std::endl;
    }
    vkDestroyImageView(this->device->device, this->view, nullptr);
    std::cout << "Destroying Image View!" << std::endl;
    for (VkImageView view : this->secondary_views)
    {
        vkDestroyImageView(this->device->device, view, nullptr);
    }
    std::cout << "Destroying secondary image view(s)!" << std::endl;
    vkDestroyImage(this->device->device, this->image, this->allocator);
    std::cout << "Destroying Image!" << std::endl;
    vkFreeMemory(this->device->device, this->memory, nullptr);
    std::cout << "Freeing Image Memory!" << std::endl;
}
