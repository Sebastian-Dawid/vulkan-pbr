#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vulkan_debug_messenger.h"
#include "vulkan_logical_device.h"
#include "vulkan_swap_chain.h"
#include "vulkan_graphics_pipeline.h"
#include "vulkan_command_buffer.h"
#include "vulkan_buffer.h"
#include "vulkan_constants.h"
#include "vulkan_descriptor_pool.h"
#include "vulkan_image.h"
#include "vulkan_render_pass.h"

#include <string>

class vulkan_context_t
{
    private:
        VkSurfaceKHR surface;
        graphics_pipeline_t* current_pipeline = nullptr;
        command_buffers_t* command_buffers = nullptr;
        std::uint32_t current_frame = 0;
        std::vector<buffer_t*> buffers;
        std::vector<image_t*> images;
        std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
        std::vector<descriptor_pool_t*> descriptor_pools;

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
        std::int32_t create_command_pool();
        std::int32_t create_sync_objects();
        std::int32_t recreate_swap_chain();
        std::int32_t create_descriptor_pool();
        std::int32_t create_descriptor_sets();

        static void framebuffer_resize_callback(GLFWwindow* window, std::int32_t width, std::int32_t height);

    public:
        GLFWwindow* window;
        bool framebuffer_resized = false;
        bool initialized = false;
        VkSampleCountFlagBits msaa_samples = VK_SAMPLE_COUNT_1_BIT;
        VkInstance instance;
        VkPhysicalDevice physical_device = VK_NULL_HANDLE;
        logical_device_t* device = nullptr;
        VkCommandPool command_pool;
        swap_chain_t* swap_chain = nullptr;
        std::vector<render_pass_t*> render_passes;
        std::vector<graphics_pipeline_t*> graphics_pipelines;
        image_t* color_buffer = nullptr;
        image_t* depth_buffer = nullptr;

        std::int32_t add_descriptor_set_layout(const std::vector<VkDescriptorSetLayoutBinding> layout_bindings = { UBO_LAYOUT_BINDING, SAMPLER_LAYOUT_BINDING });
        std::int32_t add_pipeline(const pipeline_shaders_t& shaders, const pipeline_settings_t& settings);
        std::int32_t add_buffer(const buffer_settings_t& settings);
        std::int32_t add_image(const std::string& path, const image_settings_t& settings, bool flip = false);
        std::optional<VkFramebuffer> add_framebuffer(VkRenderPass render_passs, std::vector<VkImageView> attachemnts);
        buffer_t* get_buffer(std::uint32_t index);
        buffer_t* get_last_buffer();
        std::uint32_t get_buffer_count();
        image_t* get_image(std::uint32_t index);
        std::vector<VkDescriptorSetLayout> get_descriptor_set_layouts();
        std::uint32_t get_current_frame();
        std::vector<descriptor_pool_t*>* get_descriptor_pools();
        void bind_descriptor_sets(VkCommandBuffer command_buffer, std::uint32_t pool_index, std::uint32_t first_set);
        
        std::int32_t draw_frame(std::function<void(VkCommandBuffer, vulkan_context_t*)> func);
        void main_loop(std::function<void()> func);
        VkExtent2D get_swap_chain_extent();
        vulkan_context_t(std::string name);
        ~vulkan_context_t();
};

std::vector<const char*> get_required_extensions();
