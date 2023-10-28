#include "vulkan_graphics_pipeline.h"
#include "vulkan_vertex.h"
#include <fstream>
#include <iostream>
#include <tuple>

void pipeline_settings_t::populate_defaults(const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts, render_pass_t* render_pass, std::uint32_t color_attachment_count)
{
    this->vertex_binding_descriptions.push_back(vertex_t::get_binding_description());
    std::array<VkVertexInputAttributeDescription, 4> attribute_description = vertex_t::get_attribute_description();
    this->vertex_attribute_descriptions.insert(this->vertex_attribute_descriptions.begin(), attribute_description.begin(), attribute_description.end());

    this->vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    this->vertex_input.vertexBindingDescriptionCount = 1;
    this->vertex_input.pVertexBindingDescriptions = this->vertex_binding_descriptions.data();
    this->vertex_input.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(this->vertex_attribute_descriptions.size());
    this->vertex_input.pVertexAttributeDescriptions = this->vertex_attribute_descriptions.data();

    this->input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    this->input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    this->input_assembly.primitiveRestartEnable = VK_FALSE;

    this->viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    this->viewport_state.viewportCount = 1;
    this->viewport_state.scissorCount = 1;

    this->rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    this->rasterizer.depthClampEnable = VK_FALSE;
    this->rasterizer.rasterizerDiscardEnable = VK_FALSE;
    this->rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    this->rasterizer.lineWidth = 1.0f;
    this->rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    this->rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    this->rasterizer.depthBiasEnable = VK_FALSE;

    this->multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    this->multisampling.sampleShadingEnable = VK_TRUE;
    this->multisampling.minSampleShading = .2f;
    this->multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    for (std::uint32_t i = 0; i < color_attachment_count; ++i)
    {
        VkPipelineColorBlendAttachmentState color_blend_attachment{};
        color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        color_blend_attachment.blendEnable = VK_FALSE;
        this->color_blend_attachments.push_back(color_blend_attachment);
    }

    this->color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    this->color_blending.logicOpEnable = VK_FALSE;
    this->color_blending.attachmentCount = static_cast<std::uint32_t>(color_blend_attachments.size());
    this->color_blending.pAttachments = color_blend_attachments.data();

    this->dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    this->dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    this->dynamic_state.dynamicStateCount = static_cast<std::uint32_t>(dynamic_states.size());
    this->dynamic_state.pDynamicStates = dynamic_states.data();

    this->descriptor_set_layouts = descriptor_set_layouts;

    this->depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    this->depth_stencil.depthTestEnable = VK_TRUE;
    this->depth_stencil.depthWriteEnable = VK_TRUE;
    this->depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
    this->depth_stencil.depthBoundsTestEnable = VK_FALSE;
    this->depth_stencil.minDepthBounds = 0.0f;
    this->depth_stencil.maxDepthBounds = 1.0f;
    this->depth_stencil.stencilTestEnable = VK_FALSE;
    this->depth_stencil.front = {};
    this->depth_stencil.back = {};

    this->render_pass = render_pass;
}

std::optional<std::vector<char>> read_file(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file!" << std::endl;
        return std::nullopt;
    }

    std::size_t file_size = (std::size_t) file.tellg();
    std::vector<char> buffer(file_size);
    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();
    return buffer;
}

std::optional<VkShaderModule> graphics_pipeline_t::create_shader_module(const std::vector<char>& code, VkDevice device)
{
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const std::uint32_t*>(code.data());

    VkShaderModule module;
    if (vkCreateShaderModule(device, &create_info, nullptr, &module) != VK_SUCCESS)
    {
        std::cerr << "Failed to create shader module!" << std::endl;
        return std::nullopt;
    }
    return module;
}

std::int32_t graphics_pipeline_t::init(const pipeline_shaders_t& shaders, const pipeline_settings_t& settings, const logical_device_t* device)
{
    std::optional<std::vector<char>> vertex_code = shaders.vertex.has_value() ? read_file(shaders.vertex.value()) : std::nullopt;
    std::optional<std::vector<char>> geometry_code = shaders.geometry.has_value() ? read_file(shaders.geometry.value()) : std::nullopt;
    std::optional<std::vector<char>> fragment_code = shaders.fragment.has_value() ? read_file(shaders.fragment.value()) : std::nullopt;
    
    std::vector<std::tuple<VkShaderModule, VkShaderStageFlagBits>> modules;
    std::optional<VkShaderModule> vertex_module = shaders.vertex.has_value() ? create_shader_module(vertex_code.value(), device->device) : std::nullopt;
    std::optional<VkShaderModule> geometry_module = shaders.geometry.has_value() ? create_shader_module(geometry_code.value(), device->device) : std::nullopt;
    std::optional<VkShaderModule> fragment_module = shaders.fragment.has_value() ? create_shader_module(fragment_code.value(), device->device) : std::nullopt;

    if (shaders.vertex.has_value())
    {
        if (!vertex_module.has_value()) return -1;
        modules.push_back(std::make_tuple(vertex_module.value(), VK_SHADER_STAGE_VERTEX_BIT));
    }
    if (shaders.geometry.has_value())
    {
        if (!geometry_module.has_value()) return -1;
        modules.push_back(std::make_tuple(geometry_module.value(), VK_SHADER_STAGE_GEOMETRY_BIT));
    }
    if (shaders.fragment.has_value())
    {
        if (!fragment_module.has_value()) return -1;
        modules.push_back(std::make_tuple(fragment_module.value(), VK_SHADER_STAGE_FRAGMENT_BIT));
    }

    std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
    for (std::tuple<VkShaderModule, VkShaderStageFlagBits> module : modules)
    {
        VkPipelineShaderStageCreateInfo stage{};
        stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage.stage = std::get<1>(module);
        stage.module = std::get<0>(module);
        stage.pName = "main";
        shader_stages.push_back(stage);
    }

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = static_cast<std::uint32_t>(settings.descriptor_set_layouts.size());
    pipeline_layout_info.pSetLayouts = settings.descriptor_set_layouts.data();
    pipeline_layout_info.pushConstantRangeCount = static_cast<std::uint32_t>(settings.push_constant_ranges.size());
    pipeline_layout_info.pPushConstantRanges = settings.push_constant_ranges.data();

    if (vkCreatePipelineLayout(device->device, &pipeline_layout_info, nullptr, &(this->pipeline_layout)) != VK_SUCCESS)
    {
        std::cerr << "Failed to create pipeline layout!" << std::endl;
        return -1;
    }

    VkGraphicsPipelineCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    create_info.stageCount = static_cast<std::uint32_t>(shader_stages.size());
    create_info.pStages = shader_stages.data();
    create_info.pVertexInputState = &settings.vertex_input;
    create_info.pInputAssemblyState = &settings.input_assembly;
    create_info.pViewportState = &settings.viewport_state;
    create_info.pRasterizationState = &settings.rasterizer;
    create_info.pMultisampleState = &settings.multisampling;
    create_info.pColorBlendState = &settings.color_blending;
    create_info.pDepthStencilState = &settings.depth_stencil;
    create_info.pDynamicState = &settings.dynamic_state;
    create_info.layout = this->pipeline_layout;
    create_info.renderPass = this->render_pass->render_pass;
    create_info.subpass = settings.subpass;

    if (vkCreateGraphicsPipelines(device->device, VK_NULL_HANDLE, 1, &create_info, this->allocator, &(this->pipeline)) != VK_SUCCESS)
    {
        std::cerr << "Faiuled to create graphics pipeline!" << std::endl;
        return -1;
    }

    if (vertex_module.has_value()) vkDestroyShaderModule(device->device, vertex_module.value(), nullptr);
    if (geometry_module.has_value()) vkDestroyShaderModule(device->device, geometry_module.value(), nullptr);
    if (fragment_module.has_value()) vkDestroyShaderModule(device->device, fragment_module.value(), nullptr);

    this->device = &(device->device);

    return 0;
}

graphics_pipeline_t::graphics_pipeline_t(const render_pass_t* render_pass, VkExtent2D swap_chain_extent)
{
    this->render_pass = render_pass;
    this->swap_chain_extent = swap_chain_extent;
}

graphics_pipeline_t::~graphics_pipeline_t()
{
    if (this->device == nullptr)
    {
        return;
    }
    vkDestroyPipeline(*(this->device), this->pipeline, this->allocator);
    std::cout << "Destroying Graphics Pipeline!" << std::endl;
    vkDestroyPipelineLayout(*(this->device), this->pipeline_layout, nullptr);
    std::cout << "Destroying Pipeline Layout!" << std::endl;
}
