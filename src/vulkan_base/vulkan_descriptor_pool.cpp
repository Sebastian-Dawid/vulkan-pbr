#include "vulkan_descriptor_pool.h"
#include "vulkan_buffer.h"
#include "vulkan_constants.h"
#include "vulkan_image.h"
#include <iostream>
#include <tuple>

void descriptor_pool_t::configure_descriptors(std::vector<std::tuple<std::uint32_t, VkDeviceSize, void*, VkDescriptorType, bool>> data)
{
    for (std::uint32_t i = 0; i < this->sets.size(); ++i)
    {
        std::vector<VkWriteDescriptorSet> descriptor_writes;
        std::vector<VkDescriptorBufferInfo> buffer_infos(static_cast<std::uint32_t>(data.size()));
        std::vector<VkDescriptorBufferInfo>::iterator buf_iter = buffer_infos.begin();
        std::vector<VkDescriptorImageInfo> image_infos(static_cast<std::uint32_t>(data.size()));
        std::vector<VkDescriptorImageInfo>::iterator img_iter = image_infos.begin();
        for (const auto& e : data)
        {
            VkWriteDescriptorSet descriptor_write{};
            descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write.dstSet = this->sets[i];
            descriptor_write.dstBinding = std::get<0>(e);
            descriptor_write.dstArrayElement = 0;
            descriptor_write.descriptorType = std::get<3>(e);
            descriptor_write.descriptorCount = (std::get<2>(e) == nullptr) ? 0 : 1;
            
            switch (std::get<3>(e))
            {
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    {
                        image_t* img = (image_t*) std::get<2>(e) + std::get<4>(e) * i;
                        VkDescriptorImageInfo image_info{};
                        image_info.sampler = img->sampler;
                        image_info.imageView = img->view;
                        image_info.imageLayout = img->layout;
                        *img_iter = image_info;
                    }
                    descriptor_write.pImageInfo = img_iter.base();
                    img_iter++;
                    break;
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    {
                        buffer_t* buf = *((buffer_t**) std::get<2>(e) + std::get<4>(e) * i);
                        VkDescriptorBufferInfo buffer_info{};
                        buffer_info.buffer = buf->buffer;
                        buffer_info.range = std::get<1>(e);
                        buffer_info.offset = 0;
                        *buf_iter = buffer_info;
                    }
                    descriptor_write.pBufferInfo = buf_iter.base();
                    buf_iter++;
                    break;
                default:
                    break;
            }

            descriptor_writes.push_back(descriptor_write);
        }

        vkUpdateDescriptorSets(*this->device, static_cast<std::uint32_t>(descriptor_writes.size()), descriptor_writes.data(), 0, nullptr);
    }
}

std::int32_t descriptor_pool_t::init(VkDescriptorSetLayout layout, std::vector<VkDescriptorType> types, const VkDevice* device)
{
    std::vector<VkDescriptorPoolSize> sizes;
    for (const VkDescriptorType& type : types)
    {
        VkDescriptorPoolSize pool_size{};
        pool_size.type = type;
        pool_size.descriptorCount = static_cast<std::uint32_t>(MAX_FRAMES_IN_FLIGHT);
        sizes.push_back(pool_size);
    }

    VkDescriptorPoolCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.poolSizeCount = static_cast<std::uint32_t>(sizes.size());
    create_info.pPoolSizes = sizes.data();
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
