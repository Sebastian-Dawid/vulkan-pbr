#include "vulkan_buffer.h"
#include "vulkan_command_buffer.h"
#include "vulkan_vertex.h"
#include <cstring>
#include <iostream>

#include "debug_print.h"

void buffer_settings_t::populate_defaults(std::uint32_t nr_vertices, VkBufferUsageFlags usage)
{
    this->size = sizeof(vertex_t) * nr_vertices;
    this->usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage;
    this->sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    this->memory_offset = 0;
    this->map_memory_flags = 0;
}

std::optional<std::uint32_t> find_memory_type(std::uint32_t type_filter, VkPhysicalDevice physical_device, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

    for (std::uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i)
    {
        if (type_filter & (1 << i) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    return std::nullopt;
}

std::int32_t buffer_t::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory)
{
    VkBufferCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = size;
    create_info.usage = usage;
    create_info.sharingMode = this->settings->sharing_mode;

    if (vkCreateBuffer(this->device->device, &create_info, this->allocator, &buffer) != VK_SUCCESS)
    {
        std::cerr << "Failed to create buffer!" << std::endl;
        return -1;
    }

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(this->device->device, buffer, &mem_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    std::optional<std::uint32_t> idx = find_memory_type(mem_requirements.memoryTypeBits, *this->physical_device, properties);
    if (!idx.has_value())
    {
        std::cerr << "Failed to find suitable memory type!" << std::endl;
        return -1;
    }
    alloc_info.memoryTypeIndex = idx.value();

    if (vkAllocateMemory(this->device->device, &alloc_info, nullptr, &memory) != VK_SUCCESS)
    {
        std::cerr << "Failed to allocate buffer memory!" << std::endl;
        return -1;
    }

    vkBindBufferMemory(this->device->device, buffer, memory, this->settings->memory_offset);
    
    return 0;
}

void buffer_t::copy_buffer(VkBuffer& src_buffer, VkBuffer& dst_buffer, VkDeviceSize size)
{
    VkCommandBuffer command_buffer = begin_single_time_commands(this->device->device, *this->command_pool);

    VkBufferCopy copy_region{};
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

    end_single_time_commands(*this->command_pool, command_buffer, this->device->device, this->device->graphics_queue);
}

std::int32_t buffer_t::set_data(void* cpu_data)
{
    if (this->device == nullptr)
    {
        std::cerr << "No buffer initialized!" << std::endl;
        return -1;
    }
    
    vkMapMemory(this->device->device, this->memory, this->settings->memory_offset, this->settings->size, this->settings->map_memory_flags, &this->mapped_memory);
    std::memcpy(this->mapped_memory, cpu_data, (std::size_t) this->settings->size);
    vkUnmapMemory(this->device->device, this->memory);

    return 0;
}

void buffer_t::map_memory()
{
    vkMapMemory(this->device->device, this->memory, this->settings->memory_offset, this->settings->size, this->settings->map_memory_flags, &this->mapped_memory);
}

std::int32_t buffer_t::set_staged_data(void* cpu_data)
{
    if (this->device == nullptr)
    {
        std::cerr << "No buffer initialized!" << std::endl;
        return -1;
    }

    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory;
    if (create_buffer(settings->size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                staging_buffer, staging_memory) != 0)
    {
        return -1;
    }

    void* gpu_data;
    vkMapMemory(this->device->device, staging_memory, this->settings->memory_offset, this->settings->size, this->settings->map_memory_flags, &gpu_data);
    std::memcpy(gpu_data, cpu_data, (std::size_t) this->settings->size);
    vkUnmapMemory(this->device->device, staging_memory);

    copy_buffer(staging_buffer, this->buffer, this->settings->size);

    vkDestroyBuffer(this->device->device, staging_buffer, this->allocator);
    vkFreeMemory(this->device->device, staging_memory, nullptr);

    return 0;
}

std::int32_t buffer_t::init(const buffer_settings_t& settings, const logical_device_t* device)
{
    this->device = device;
    this->settings = &settings;

    if (create_buffer(settings.size, settings.usage, settings.memory_properties, this->buffer, this->memory) != 0)
    {
        this->device = nullptr;
        return -1;
    }

    return 0;    
}

buffer_t::buffer_t(const VkPhysicalDevice* physical_device, const VkCommandPool* command_pool)
{
    this->physical_device = physical_device;
    this->command_pool = command_pool;
}

buffer_t::~buffer_t()
{
    if (this->device == nullptr) return;
    vkDestroyBuffer(this->device->device, this->buffer, this->allocator);
    DEBUG_PRINT("Destroying Buffer!");
    vkFreeMemory(this->device->device, this->memory, nullptr);
    DEBUG_PRINT("Freeing Buffer Memory!");
}

buffer_settings_t buffer_t::get_settings()
{
    return *this->settings;
}
