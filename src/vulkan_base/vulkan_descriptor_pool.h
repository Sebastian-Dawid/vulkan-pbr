#pragma once

#include <vector>
#include <cstdint>
#include <vulkan/vulkan_core.h>

class descriptor_pool_t
{
    private:
        const VkDevice* device = nullptr;
        const VkAllocationCallbacks* allocator = nullptr;
        VkDescriptorType type;
    public:
        VkDescriptorPool pool;
        std::vector<VkDescriptorSet> sets;

        void configure_descriptors(std::uint32_t binding, VkBuffer buffer, VkDeviceSize size);
        std::int32_t init(VkDescriptorType type, VkDescriptorSetLayout layout, const VkDevice* device);
        descriptor_pool_t();
        ~descriptor_pool_t();
};
