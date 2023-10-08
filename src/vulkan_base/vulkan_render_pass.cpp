#include "vulkan_render_pass.h"
#include "vulkan_image.h"
#include <algorithm>
#include <iostream>

void render_pass_settings_t::add_subpass(VkFormat format, VkSampleCountFlagBits msaa_samples, const VkPhysicalDevice* physical_device,
        std::uint32_t color_attachment_count, std::uint32_t depth_attachment_count, std::uint32_t color_resolve_attachment_count,
        std::uint32_t input_attachment_count)
{
    this->subpasses.push_back(subpass_t());
    subpass_t& subpass = this->subpasses.back();
    for (std::uint32_t i = 0; i < color_attachment_count; ++i)
    {
        VkAttachmentDescription color_attachment{};
        color_attachment.format = format;
        color_attachment.samples = msaa_samples;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            
        VkAttachmentReference color_attachment_ref{};
        color_attachment_ref.attachment = static_cast<std::uint32_t>(this->attachments.size());
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        subpass.color_attachment_references.push_back(color_attachment_ref);
        this->attachments.push_back(color_attachment);
    }

    subpass.description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.description.colorAttachmentCount = static_cast<std::uint32_t>(subpass.color_attachment_references.size());
    subpass.description.pColorAttachments = subpass.color_attachment_references.data();

    if (depth_attachment_count > 0)
    {
        VkAttachmentDescription depth_attachment{};
        depth_attachment.format = find_depth_format(physical_device).value();
        depth_attachment.samples = msaa_samples;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        subpass.depth_attachment_reference[0].attachment = static_cast<std::uint32_t>(this->attachments.size());
        subpass.depth_attachment_reference[0].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        subpass.description.pDepthStencilAttachment = subpass.depth_attachment_reference.data();

        this->attachments.push_back(depth_attachment);
    }
    
    if (color_resolve_attachment_count > 0)
    {
        VkAttachmentDescription color_attachment_resolve{};
        color_attachment_resolve.format = format;
        color_attachment_resolve.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment_resolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment_resolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment_resolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment_resolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment_resolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment_resolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference color_attachment_resolve_reference{};
        color_attachment_resolve_reference.attachment = static_cast<std::uint32_t>(this->attachments.size());
        color_attachment_resolve_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
        subpass.color_attachment_resolve_references.push_back(color_attachment_resolve_reference);
        subpass.description.pResolveAttachments = subpass.color_attachment_resolve_references.data();
        this->attachments.push_back(color_attachment_resolve);
    }

    for (std::uint32_t i = 0; i < input_attachment_count; ++i)
    {
        VkAttachmentReference input_attachment_reference{};
        input_attachment_reference.attachment = i;
        input_attachment_reference.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        subpass.input_attachment_references.push_back(input_attachment_reference);
        subpass.description.pInputAttachments = subpass.input_attachment_references.data();
    }
    subpass.description.inputAttachmentCount = static_cast<std::uint32_t>(input_attachment_count);

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    this->dependencies.push_back(dependency);
}

std::int32_t render_pass_t::init(const render_pass_settings_t& settings, const VkDevice& device)
{
    VkRenderPassCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount = static_cast<std::uint32_t>(settings.attachments.size());
    create_info.pAttachments = settings.attachments.data();
    create_info.subpassCount = static_cast<std::uint32_t>(settings.subpasses.size());
    std::vector<VkSubpassDescription> subpasses;
    std::for_each(settings.subpasses.begin(), settings.subpasses.end(), [&] (subpass_t s) { subpasses.push_back(s.description); });
    create_info.pSubpasses = subpasses.data();
    create_info.dependencyCount = static_cast<std::uint32_t>(settings.dependencies.size());
    create_info.pDependencies = settings.dependencies.data();

    if (vkCreateRenderPass(device, &create_info, nullptr, &this->render_pass) != VK_SUCCESS)
    {
        std::cerr << "Failed to create render pass!" << std::endl;
        return -1;
    }

    this->subpass_count = static_cast<std::uint32_t>(settings.subpasses.size());
    this->framebuffers.resize(this->subpass_count);
    this->device = &device;

    return 0;
}

std::int32_t render_pass_t::add_framebuffer(std::uint32_t width, std::uint32_t height, std::vector<framebuffer_attachment_t> attachments)
{
    VkFramebuffer fb;
    std::vector<VkImageView> attachments_views(static_cast<std::uint32_t>(attachments.size()));
    std::uint32_t index = 0;
    for (framebuffer_attachment_t attachment : attachments)
    {
        switch (attachment.type)
        {
            case IMAGE:
                attachments_views[index] = ((*((image_t**) attachment.source))->view);
                break;
            case SWAP_CHAIN:
                attachments_views[index] = ((*((swap_chain_t**) attachment.source))->image_views[attachment.index]);
                break;
        }
        ++index;
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
    this->framebuffers[this->current_subpass].push_back({width, height, fb, attachments});

    return 0;
}

std::int32_t render_pass_t::recreate_framebuffer(std::uint32_t index, std::uint32_t width, std::uint32_t height)
{
    vkDestroyFramebuffer(*this->device, this->framebuffers[this->current_subpass][index].framebuffer, nullptr);
    
    std::vector<VkImageView> attachments_views;
    for (framebuffer_attachment_t attachment : this->framebuffers[this->current_subpass][index].attachments)
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
    create_info.width = (this->resizeable) ? width : this->framebuffers[this->current_subpass][index].width;
    create_info.height = (this->resizeable) ? height : this->framebuffers[this->current_subpass][index].height;
    create_info.layers = 1;
    if (vkCreateFramebuffer(*this->device, &create_info, nullptr, &this->framebuffers[this->current_subpass][index].framebuffer) != VK_SUCCESS)
    {
        std::cerr << "Failed to create framebuffer!" << std::endl;
        return -1;
    }

    return 0;
}

render_pass_t::~render_pass_t()
{
    if (this->device == nullptr) return;
    for (std::vector<framebuffer_t> fbs : this->framebuffers)
    {
        std::for_each(fbs.begin(), fbs.end(), [&](framebuffer_t& fb){ vkDestroyFramebuffer(*this->device, fb.framebuffer, nullptr); });
    }
    std::cout << "Destroying Framebuffer(s)!" << std::endl;
    vkDestroyRenderPass(*this->device, this->render_pass, nullptr);
    std::cout << "Destroying Render Pass!" << std::endl;
}
