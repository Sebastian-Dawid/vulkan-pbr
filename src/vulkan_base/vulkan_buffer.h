#pragma once

#include <cstdint>
#include <optional>
#include <vulkan/vulkan_core.h>
#include "vulkan_logical_device.h"

struct buffer_settings_t
{
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkSharingMode sharing_mode;
    VkDeviceSize memory_offset;
    VkMemoryMapFlags map_memory_flags;

    void populate_defaults(std::uint32_t nr_vertices, VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
};

class buffer_t
{
    private:
        const VkPhysicalDevice* physical_device = nullptr;
        const VkCommandPool* command_pool = nullptr;
        const logical_device_t* device = nullptr;
        const VkAllocationCallbacks* allocator = nullptr;
        const buffer_settings_t* settings;

        std::optional<std::uint32_t> find_memory_type(std::uint32_t type_filter, VkMemoryPropertyFlags properties);
        std::int32_t create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory);
        void copy_buffer(VkBuffer& src_buffer, VkBuffer& dst_buffer, VkDeviceSize size);

    public:
        VkBuffer buffer;
        VkDeviceMemory memory;
        
        std::int32_t set_data(void* cpu_data);
        std::int32_t init(const buffer_settings_t& settings, const logical_device_t* device);
        buffer_t(const VkPhysicalDevice* physical_device, const VkCommandPool* command_pool);
        ~buffer_t();
};
