#include "vulkan_debug_messenger.h"
#include <iostream>

#include "debug_print.h"

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
{
    std::cerr << "validation layer: " << callback_data->pMessage << std::endl;
    return VK_FALSE;
}

void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT &create_info)
{
    create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = debug_callback;
    create_info.pUserData = nullptr;
}

VkResult debug_messenger_t::init(VkInstance* instance, const VkAllocationCallbacks* allocator)
{
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(*instance, "vkCreateDebugUtilsMessengerEXT");
    VkResult result;
    if (func != nullptr)
    {
        result = func(*instance, &(this->create_info), allocator, &(this->debug_messenger));
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    if (result == VK_SUCCESS)
    {
        this->initialized = true;
        this->instance = instance;
        this->allocator = allocator;
    }

    return result;
}

debug_messenger_t::debug_messenger_t()
{
    populate_debug_messenger_create_info(this->create_info);
}

debug_messenger_t::~debug_messenger_t()
{
    if (!initialized)
    {
        return;
    }
    
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(*(this->instance), "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(*(this->instance), this->debug_messenger, this->allocator);
    }
    DEBUG_PRINT("Destroying Debug Messenger!")
}
