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

#include <string>

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

        debug_messenger_t* debug_messenger;
        std::int32_t create_instance(std::string name);
        std::int32_t pick_physical_device();
        std::int32_t create_surface();
        std::int32_t create_render_pass();

    public:
        GLFWwindow* window;
        bool initialized = false;
        vulkan_context_t(std::string name);
        ~vulkan_context_t();
};

std::vector<const char*> get_required_extensions();
