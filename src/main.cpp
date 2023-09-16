#include "vulkan_base.h"

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    vulkan_context_t vk_context("Vulkan Template");
    vk_context.window = glfwCreateWindow(1280, 720, "Vulkan Template", nullptr, nullptr);

    while (!glfwWindowShouldClose(vk_context.window))
    {
        glfwPollEvents();
    }

    glfwDestroyWindow(vk_context.window);
    glfwTerminate();
    return 0;
}
