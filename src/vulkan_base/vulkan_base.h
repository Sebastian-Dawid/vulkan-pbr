#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "vulkan_debug_messenger.h"
#include "vulkan_logical_device.h"
#include "vulkan_swap_chain.h"
#include "vulkan_graphics_pipeline.h"
#include "vulkan_command_buffer.h"

#include <string>

struct render_pass_settings_t
{
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> attachment_references;
    std::vector<VkSubpassDescription> subpasses;
    std::vector<VkSubpassDependency> dependencies;

    void populate_defaults(VkFormat format);
};

class vulkan_context_t
{
    private:
        VkInstance instance;
        VkPhysicalDevice physical_device = VK_NULL_HANDLE;
        logical_device_t* device;
        VkSurfaceKHR surface;
        swap_chain_t* swap_chain;
        VkRenderPass render_pass;
        graphics_pipeline_t* graphics_pipeline;
        std::vector<VkFramebuffer> swap_chain_framebuffers;
        VkCommandPool command_pool;
        command_buffers_t* command_buffers;
        std::uint32_t current_frame = 0;

        struct
        {
            std::vector<VkSemaphore> image_available;
            std::vector<VkSemaphore> render_finished;
            std::vector<VkFence> in_flight;
        } sync_objects;

        debug_messenger_t* debug_messenger;
        std::int32_t create_instance(std::string name);
        std::int32_t pick_physical_device();
        std::int32_t create_surface();
        std::int32_t create_render_pass(const render_pass_settings_t& settings);
        std::int32_t create_framebuffers();
        std::int32_t create_command_pool();
        std::int32_t create_sync_objects();
        std::int32_t recreate_swap_chain();

        static void framebuffer_resize_callback(GLFWwindow* window, std::int32_t width, std::int32_t height);

    public:
        GLFWwindow* window;
        bool framebuffer_resized = false;
        bool initialized = false;
        std::int32_t draw_frame(std::function<void(VkCommandBuffer, vulkan_context_t*)> func);
        void main_loop(std::function<void()> func);
        VkExtent2D get_swap_chain_extent();
        vulkan_context_t(std::string name);
        ~vulkan_context_t();
};

std::vector<const char*> get_required_extensions();
