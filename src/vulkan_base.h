#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <string>

class vulkan_context_t
{
    private:
        VkInstance instance;
        std::int32_t create_instance(std::string name);

    public:
        GLFWwindow* window;
        vulkan_context_t(std::string name);
        ~vulkan_context_t();
};
