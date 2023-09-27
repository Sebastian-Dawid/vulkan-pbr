#include "vulkan_descriptor_pool.h"
#include "vulkan_constants.h"
#include <iostream>

void descriptor_pool_t::configure_descriptors(std::uint32_t binding, VkBuffer buffer, VkDeviceSize size)
{
    for (VkDescriptorSet set : this->sets)
    {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = buffer;
        buffer_info.range = size;
        buffer_info.offset = 0;

        VkWriteDescriptorSet descriptor_write{};
        descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstSet = set;
        descriptor_write.dstBinding = binding;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType = this->type;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pBufferInfo = &buffer_info;

        vkUpdateDescriptorSets(*this->device, 1, &descriptor_write, 0, nullptr);
    }
}

std::int32_t descriptor_pool_t::init(VkDescriptorType type, VkDescriptorSetLayout layout, const VkDevice* device)
{
    VkDescriptorPoolSize pool_size{};
    pool_size.type = type;
    pool_size.descriptorCount = static_cast<std::uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.poolSizeCount = 1;
    create_info.pPoolSizes = &pool_size;
    create_info.maxSets = static_cast<std::uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(*device, &create_info, nullptr, &this->pool) != VK_SUCCESS)
    {
        std::cerr << "Failed to create descriptor pool!" << std::endl;
        return -1;
    }

    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, layout);
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = this->pool;
    alloc_info.descriptorSetCount = static_cast<std::uint32_t>(MAX_FRAMES_IN_FLIGHT);
    alloc_info.pSetLayouts = layouts.data();

    this->sets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(*device, &alloc_info, this->sets.data()) != VK_SUCCESS)
    {
        std::cerr << "Failed to allocate descriptor sets!" << std::endl;
        return -1;
    }

    this->type = type;
    this->device = device;
    return 0;
}

descriptor_pool_t::descriptor_pool_t()
{}

descriptor_pool_t::~descriptor_pool_t()
{
    if (this->device == nullptr) return;
    vkDestroyDescriptorPool(*this->device, this->pool, this->allocator);
    std::cout << "Destroying Descriptor Pool!" << std::endl;
}
