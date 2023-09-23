#pragma once

#include <vector>
#include <cstdint>
#include <vulkan/vulkan_core.h>

inline const std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
inline const std::uint32_t MAX_FRAMES_IN_FLIGHT = 2;
