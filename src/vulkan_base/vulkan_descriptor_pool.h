#pragma once

#include <vector>
#include <cstdint>
#include <vulkan/vulkan_core.h>

class descriptor_pool_t
{
    private:
        const VkDevice* device = nullptr;
        const VkAllocationCallbacks* allocator = nullptr;
    public:
        VkDescriptorPool pool;
        std::vector<VkDescriptorSet> sets;

        void configure_descriptors(std::vector<std::tuple<std::uint32_t, VkDeviceSize, void*, VkDescriptorType, bool>> data);
        std::int32_t init(VkDescriptorSetLayout layout, std::vector<VkDescriptorType> types, const VkDevice* device);
        descriptor_pool_t();
        ~descriptor_pool_t();
};
