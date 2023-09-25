#include "vulkan_buffer.h"
#include "vulkan_vertex.h"
#include <cstring>
#include <iostream>

void buffer_settings_t::populate_defaults(std::uint32_t nr_vertices)
{
    this->size = sizeof(vertex_t) * nr_vertices;
    this->usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    this->sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    this->memory_offset = 0;
    this->map_memory_flags = 0;
}

std::optional<std::uint32_t> buffer_t::find_memory_type(std::uint32_t type_filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(*(this->physical_device), &mem_properties);

    for (std::uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i)
    {
        if (type_filter & (1 << i) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    return std::nullopt;
}

void buffer_t::set_data(void* cpu_data)
{
    if (this->device == nullptr)
    {
        std::cerr << "No buffer initialized!" << std::endl;
        return;
    }
    void* gpu_data;
    vkMapMemory(*this->device, this->memory, this->settings->memory_offset, this->settings->size, this->settings->map_memory_flags, &gpu_data);
    std::memcpy(gpu_data, cpu_data, (std::size_t) this->settings->size);
    vkUnmapMemory(*this->device, this->memory);
}

std::int32_t buffer_t::init(const buffer_settings_t& settings, const VkDevice* device)
{
    VkBufferCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = settings.size;
    create_info.usage = settings.usage;
    create_info.sharingMode = settings.sharing_mode;

    if (vkCreateBuffer(*device, &create_info, this->allocator, &this->buffer) != VK_SUCCESS)
    {
        std::cerr << "Failed to create buffer!" << std::endl;
        return -1;
    }

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(*device, this->buffer, &mem_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    std::optional<std::uint32_t> idx = find_memory_type(mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!idx.has_value())
    {
        std::cerr << "Failed to find suitable memory type!" << std::endl;
        return -1;
    }
    alloc_info.memoryTypeIndex = idx.value();

    if (vkAllocateMemory(*device, &alloc_info, nullptr, &this->memory) != VK_SUCCESS)
    {
        std::cerr << "Failed to allocate buffer memory!" << std::endl;
        return -1;
    }

    vkBindBufferMemory(*device, this->buffer, this->memory, settings.memory_offset);
    this->device = device;
    this->settings = &settings;

    return 0;    
}

buffer_t::buffer_t(const VkPhysicalDevice* physical_device)
{
    this->physical_device = physical_device;
}

buffer_t::~buffer_t()
{
    if (this->device == nullptr) return;
    vkDestroyBuffer(*(this->device), this->buffer, this->allocator);
    std::cout << "Destroying Buffer!" << std::endl;
    vkFreeMemory(*(this->device), this->memory, nullptr);
    std::cout << "Freeing Buffer Memory!" << std::endl;
}
