#include "model_base.h"
#include <iostream>
#include <tiny_obj_loader.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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

std::int32_t model_t::tiny_obj_init(const std::string& path)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str()))
    {
        std::cerr << warn + err << std::endl;
        return -1;
    }

    std::unordered_map<vertex_t, std::uint32_t> unique_vertices{};
    std::unordered_map<vertex_t, glm::vec3> tangents{};

    for (const tinyobj::shape_t& shape : shapes)
    {
        std::uint32_t count = 0;
        std::array<vertex_t, 3> tri;
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

            tri[count] = vertex;
            count = (count == 2) ? 0 : count + 1;
            
            if (count == 0)
            {
                vertex_t& a = tri[0];
                vertex_t& b = tri[1];
                vertex_t& c = tri[2];

                glm::vec3 ab = b.pos - a.pos;
                glm::vec3 ac = c.pos - a.pos;
                glm::vec2 delta_uv_1 = b.tex_coord - a.tex_coord;
                glm::vec2 delta_uv_2 = c.tex_coord - a.tex_coord;

                float f = 1.0f / (delta_uv_1.x * delta_uv_2.y - delta_uv_2.x * delta_uv_1.y);

                glm::vec3 tangent(
                        f * (delta_uv_2.y * ab.x - delta_uv_1.y * ac.x),
                        f * (delta_uv_2.y * ab.y - delta_uv_1.y * ac.y),
                        f * (delta_uv_2.y * ab.z - delta_uv_1.y * ac.z)
                        );
                a.tangent = tangent;
                b.tangent = tangent;
                c.tangent = tangent;

                for (vertex_t vert : tri)
                {
                    if (unique_vertices.count(vert) == 0)
                    {
                        unique_vertices[vert] = static_cast<std::uint32_t>(vertices.size());
                        this->vertices.push_back(vert);
                    }

                    tangents[vert] += tangent;
                    this->indices.push_back(unique_vertices[vert]);
                }
            }
        }
    }

    for (vertex_t& vert : this->vertices)
    {
        vert.tangent = glm::normalize(tangents[vert]);
    }

    this->initialized = true;
    return 0;
}

void process_node(model_t* model, aiNode* node, const aiScene* scene)
{
    for (std::uint32_t i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

        for (std::uint32_t j = 0; j < mesh->mNumVertices; ++j)
        {
            vertex_t vertex;
            glm::vec3 vec3;
            glm::vec2 vec2;
            vec3.x = mesh->mVertices[j].x;
            vec3.y = mesh->mVertices[j].y;
            vec3.z = mesh->mVertices[j].z;
            vertex.pos = vec3;

            vec3.x = mesh->mNormals[j].x;
            vec3.y = mesh->mNormals[j].y;
            vec3.z = mesh->mNormals[j].z;
            vertex.normal = vec3;

            if (mesh->mTextureCoords[0])
            {
                vec2.x = mesh->mTextureCoords[0][j].x;
                vec2.y = mesh->mTextureCoords[0][j].y;
                vertex.tex_coord = vec2;
            
                vec3.x = mesh->mTangents[j].x;
                vec3.y = mesh->mTangents[j].y;
                vec3.z = mesh->mTangents[j].z;
                vertex.tangent = vec3;
            }

            model->vertices.push_back(vertex);
        }

        std::uint32_t offset = model->vertices.size();
        for (std::uint32_t j = 0; j < mesh->mNumFaces; ++j)
        {
            aiFace face = mesh->mFaces[j];
            for (std::uint32_t k = 0; k < face.mNumIndices; ++k)
            {
                model->indices.push_back(offset + face.mIndices[k]);
            }
        }
    }

    for (std::uint32_t i = 0; i < node->mNumChildren; ++i)
    {
        process_node(model, node->mChildren[i], scene);
    }
}

std::int32_t model_t::assimp_init(const std::string& path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
        return -1;
    }

    process_node(this, scene->mRootNode, scene);

    this->initialized = true;
    return 0;
}

model_t::model_t(const std::string& path, bool assimp)
{
    if (!assimp)
    {
        tiny_obj_init(path);
        return;
    }
    assimp_init(path);
}
