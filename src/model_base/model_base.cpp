#include "model_base.h"
#include <iostream>
#include <tiny_obj_loader.h>

std::optional<std::tuple<buffer_t*, buffer_t*>> model_t::set_up_buffer(vulkan_context_t* context)
{
    buffer_settings_t vertex_buffer_settings;
    vertex_buffer_settings.populate_defaults(static_cast<std::uint32_t>(this->vertices.size()));
    if (context->add_buffer(vertex_buffer_settings) != 0) return std::nullopt;
    buffer_t* vertex_buffer = context->get_last_buffer();
    vertex_buffer->set_staged_data(this->vertices.data());
    
    buffer_settings_t index_buffer_settings;
    index_buffer_settings.populate_defaults(0, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    index_buffer_settings.size = static_cast<std::uint32_t>(this->indices.size()) * sizeof(std::uint32_t);
    if (context->add_buffer(index_buffer_settings) != 0) return std::nullopt;
    buffer_t* index_buffer = context->get_last_buffer();
    index_buffer->set_staged_data(this->indices.data());
    
    return std::make_tuple(vertex_buffer, index_buffer);
}

bool model_t::is_initialized()
{
    return this->initialized;
}

model_t::model_t(const std::string& path)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str()))
    {
        std::cerr << warn + err << std::endl;
        return;
    }

    std::unordered_map<vertex_t, std::uint32_t> unique_vertices{};

    for (const tinyobj::shape_t& shape : shapes)
    {
        for (const tinyobj::index_t& index : shape.mesh.indices)
        {
            vertex_t vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2],
            };

            vertex.tex_coord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
            };

            vertex.normal = {
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2],
            };

            if (unique_vertices.count(vertex) == 0)
            {
                unique_vertices[vertex] = static_cast<std::uint32_t>(vertices.size());
                this->vertices.push_back(vertex);
            }

            this->indices.push_back(unique_vertices[vertex]);
        }
    }

    this->initialized = true;
}
