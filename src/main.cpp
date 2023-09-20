#include "vulkan_base/vulkan_base.h"

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    vulkan_context_t vk_context("Vulkan Template");
    if (!vk_context.initialized)
    {
        glfwTerminate();
        return -1;
    }

    while (!glfwWindowShouldClose(vk_context.window))
    {
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
