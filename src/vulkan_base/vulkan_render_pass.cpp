#include "vulkan_render_pass.h"
#include "vulkan_image.h"
#include <algorithm>
#include <iostream>

std::int32_t render_pass_t::init(const render_pass_settings_t& settings, const VkDevice& device)
{
    VkRenderPassCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount = static_cast<std::uint32_t>(settings.attachments.size());
    create_info.pAttachments = settings.attachments.data();
    create_info.subpassCount = static_cast<std::uint32_t>(settings.subpasses.size());
    create_info.pSubpasses = settings.subpasses.data();
    create_info.dependencyCount = static_cast<std::uint32_t>(settings.dependencies.size());
    create_info.pDependencies = settings.dependencies.data();
    
    if (vkCreateRenderPass(device, &create_info, nullptr, &this->render_pass) != VK_SUCCESS)
    {
        std::cerr << "Failed to create render pass!" << std::endl;
        return -1;
    }

    this->device = &device;

    return 0;
}

std::int32_t render_pass_t::add_framebuffer(std::uint32_t width, std::uint32_t height, std::vector<framebuffer_attachment_t> attachments)
{
    VkFramebuffer fb;
    std::vector<VkImageView> attachments_views;
    for (framebuffer_attachment_t attachment : attachments)
    {
        switch (attachment.type)
        {
            case IMAGE:
                attachments_views.push_back((*((image_t**) attachment.source))->view);
                break;
            case SWAP_CHAIN:
                attachments_views.push_back((*((swap_chain_t**) attachment.source))->image_views[attachment.index]);
                break;
        }
    }
    VkFramebufferCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    create_info.renderPass = render_pass;
    create_info.attachmentCount = static_cast<std::uint32_t>(attachments_views.size());
    create_info.pAttachments = attachments_views.data();
    create_info.width = width;
    create_info.height = height;
    create_info.layers = 1;
    if (vkCreateFramebuffer(*this->device, &create_info, nullptr, &fb) != VK_SUCCESS)
    {
        std::cerr << "Failed to create framebuffer!" << std::endl;
        return -1;
    }
    this->framebuffers.push_back({width, height, fb, attachments});

    return 0;
}

std::int32_t render_pass_t::recreate_framebuffer(std::uint32_t index, std::uint32_t width, std::uint32_t height)
{
    vkDestroyFramebuffer(*this->device, this->framebuffers[index].framebuffer, nullptr);
    
    std::vector<VkImageView> attachments_views;
    for (framebuffer_attachment_t attachment : this->framebuffers[index].attachments)
    {
        switch (attachment.type)
        {
            case IMAGE:
                attachments_views.push_back((*((image_t**) attachment.source))->view);
                break;
            case SWAP_CHAIN:
                attachments_views.push_back((*((swap_chain_t**) attachment.source))->image_views[attachment.index]);
                break;
        }
    }

    VkFramebufferCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    create_info.renderPass = render_pass;
    create_info.attachmentCount = static_cast<std::uint32_t>(attachments_views.size());
    create_info.pAttachments = attachments_views.data();
    create_info.width = (this->resizeable) ? width : this->framebuffers[index].width;
    create_info.height = (this->resizeable) ? height : this->framebuffers[index].height;
    create_info.layers = 1;
    if (vkCreateFramebuffer(*this->device, &create_info, nullptr, &this->framebuffers[index].framebuffer) != VK_SUCCESS)
    {
        std::cerr << "Failed to create framebuffer!" << std::endl;
        return -1;
    }

    return 0;
}

render_pass_t::~render_pass_t()
{
    if (this->device == nullptr) return;
    std::for_each(this->framebuffers.begin(), this->framebuffers.end(), [&](framebuffer_t& fb){ vkDestroyFramebuffer(*this->device, fb.framebuffer, nullptr); });
    std::cout << "Destroying Framebuffer(s)!" << std::endl;
    vkDestroyRenderPass(*this->device, this->render_pass, nullptr);
    std::cout << "Destroying Render Pass!" << std::endl;
}
