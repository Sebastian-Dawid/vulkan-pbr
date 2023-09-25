#pragma once

#include <cstdint>
#include <optional>
#include <vulkan/vulkan_core.h>

struct buffer_settings_t
{
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkSharingMode sharing_mode;
    VkDeviceSize memory_offset;
    VkMemoryMapFlags map_memory_flags;

    void populate_defaults(std::uint32_t nr_vertices);
};

class buffer_t
{
    private:
        const VkPhysicalDevice* physical_device = nullptr;
        const VkDevice* device = nullptr;
        const VkAllocationCallbacks* allocator = nullptr;
        const buffer_settings_t* settings;

        std::optional<std::uint32_t> find_memory_type(std::uint32_t type_filter, VkMemoryPropertyFlags properties);

    public:
        VkBuffer buffer;
        VkDeviceMemory memory;
        
        void set_data(void* cpu_data);
        std::int32_t init(const buffer_settings_t& settings, const VkDevice* device);
        buffer_t(const VkPhysicalDevice* physical_device);
        ~buffer_t();
};
