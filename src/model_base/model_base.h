#pragma once

#include <string>
#include <vulkan_base/vulkan_base.h>
#include <vulkan_base/vulkan_vertex.h>

class model_t
{
    private:
        bool initialized = false;
        std::int32_t assimp_init(const std::string& path);
        std::int32_t tiny_obj_init(const std::string& path);
    public:
        std::vector<vertex_t> vertices;
        std::vector<std::uint32_t> indices;
        std::optional<std::tuple<buffer_t*, buffer_t*>> set_up_buffer(vulkan_context_t* context);
        bool is_initialized();
        model_t(const std::string& path, bool assimp = true);
};
