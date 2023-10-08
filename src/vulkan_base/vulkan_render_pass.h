#pragma once

#include "vulkan_swap_chain.h"
#include <vector>
#include <cstdint>
#include <vulkan/vulkan_core.h>

struct subpass_t
{
    VkSubpassDescription description{};
    std::vector<VkAttachmentReference> color_attachment_references;
    std::vector<VkAttachmentReference> depth_attachment_reference = {{}};
    std::vector<VkAttachmentReference> color_attachment_resolve_references;
};

struct render_pass_settings_t
{
    std::vector<VkAttachmentDescription> attachments;
    std::vector<subpass_t> subpasses;
    std::vector<VkSubpassDependency> dependencies;

    void add_subpass(VkFormat format, VkSampleCountFlagBits msaa_samples, const VkPhysicalDevice* physical_device,
            std::uint32_t color_attachment_count = 1, std::uint32_t depth_attachment_count = 1, std::uint32_t color_attachment_resolve_count = 1);
};

enum attachment_type_t
{
    IMAGE,
    SWAP_CHAIN
};

struct framebuffer_attachment_t
{
    void* source;
    attachment_type_t type;
    std::uint32_t index = 0;
};

struct framebuffer_t
{
    std::uint32_t width;
    std::uint32_t height;
    VkFramebuffer framebuffer;
    std::vector<framebuffer_attachment_t> attachments;
};

struct render_pass_t
{
    bool resizeable = false;
    VkRenderPass render_pass;
    std::uint32_t subpass_count = 1;
    std::uint32_t current_subpass = 0;
    std::vector<std::vector<framebuffer_t>> framebuffers;
    const VkDevice* device;

    std::int32_t init(const render_pass_settings_t& settings, const VkDevice& device);
    std::int32_t add_framebuffer(std::uint32_t width, std::uint32_t height, std::vector<framebuffer_attachment_t> attachments);
    std::int32_t recreate_framebuffer(std::uint32_t index, std::uint32_t width , std::uint32_t height);
    ~render_pass_t();
};
