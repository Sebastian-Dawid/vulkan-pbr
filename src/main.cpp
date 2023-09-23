#include "vulkan_base/vulkan_base.h"

int main()
{
    vulkan_context_t vk_context("Vulkan Template");
    if (!vk_context.initialized)
    {
        glfwTerminate();
        return -1;
    }

    vk_context.main_loop([&]
    {
        while (!glfwWindowShouldClose(vk_context.window))
        {
            glfwPollEvents();
            if (vk_context.draw_frame([]{}) != 0)
            {
                break;
            }
        }
    });

    glfwTerminate();
    return 0;
}
