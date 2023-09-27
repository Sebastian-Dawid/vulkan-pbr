#pragma once

#include <cstdint>
#include <optional>
#include <vulkan/vulkan_core.h>
#include "vulkan_logical_device.h"

struct buffer_settings_t
{
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkSharingMode sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    VkDeviceSize memory_offset = 0;
    VkMemoryMapFlags map_memory_flags = 0;
    std::uint32_t binding = 0;

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
        void* mapped_memory = nullptr;

        std::int32_t set_data(void* cpu_data);
        void map_memory();
        std::int32_t set_staged_data(void* cpu_data);
        buffer_settings_t get_settings();
        std::int32_t init(const buffer_settings_t& settings, const logical_device_t* device);
        buffer_t(const VkPhysicalDevice* physical_device, const VkCommandPool* command_pool);
        ~buffer_t();
};
