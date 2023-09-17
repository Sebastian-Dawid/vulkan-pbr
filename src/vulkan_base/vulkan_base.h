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

#include <string>

class vulkan_context_t
{
    private:
        VkInstance instance;
        debug_messenger_t* debug_messenger;
        std::int32_t create_instance(std::string name);

    public:
        GLFWwindow* window;
        bool initialized = false;
        vulkan_context_t(std::string name);
        ~vulkan_context_t();
};

std::vector<const char*> get_required_extentions();
