#include "vulkan_validation_layers.h"
#include <cstdint>
#include <cstring>
#include <vulkan/vulkan_core.h>

bool check_validation_layer_support()
{
    std::uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    for (const char* layer_name : validation_layers)
    {
        bool layer_found = false;
        for (const VkLayerProperties& layer_properties : available_layers)
        {
            if (std::strcmp(layer_properties.layerName, layer_name) == 0)
            {
                layer_found = true;
                break;
            }
        }
        if (!layer_found)
        {
            return false;
        }
    }

    return true;
}
