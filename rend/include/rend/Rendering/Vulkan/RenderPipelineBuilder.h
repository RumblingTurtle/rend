#pragma once
#include <iostream>
#include <rend/Rendering/Vulkan/DescriptorSet.h>
#include <rend/Rendering/Vulkan/Shader.h>
#include <rend/Rendering/Vulkan/vk_struct_init.h>
#include <rend/macros.h>
#include <vector>
#include <vulkan/vulkan.h>

class RenderPipelineBuilder {
  std::vector<VkPipelineShaderStageCreateInfo> _shaderStages{};
  VkPipelineVertexInputStateCreateInfo _vertexInputInfo{};
  VkPipelineInputAssemblyStateCreateInfo _inputAssembly{};
  VkPipelineRasterizationStateCreateInfo _rasterizer{};
  VkPipelineColorBlendAttachmentState _colorBlendAttachment{};
  VkPipelineMultisampleStateCreateInfo _multisampling{};
  VkPipelineColorBlendStateCreateInfo _colorBlending{};
  VkPipelineViewportStateCreateInfo _viewportState{};
  VkPipelineDepthStencilStateCreateInfo _depth_stencil_create_info{};
  VkPipelineLayoutCreateInfo pipeline_layout_info{};
  VkViewport _viewport;
  VkRect2D _scissor;

public:
  void build_pipeline_layout(VkDevice &_device,
                             VkPushConstantRange &push_constant_range,
                             DescriptorSetAllocator &ds_allocator,
                             VkPipelineLayout &layout_out) {
    pipeline_layout_info = get_pipeline_layout_create_info(ds_allocator);
    if (push_constant_range.size != 0) {
      pipeline_layout_info.pPushConstantRanges = &push_constant_range;
      pipeline_layout_info.pushConstantRangeCount = 1;
    }

    if (vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr,
                               &layout_out) != VK_SUCCESS) {
      std::cerr << "Failed to create pipeline layout" << std::endl;
    }
  }

  void build_pipeline(VkDevice &device, VkRenderPass &render_pass,
                      VkExtent2D extent, Shader &shader,
                      VkPipelineLayout &_pipeline_layout,
                      VertexInfoDescription &vertex_info_description,
                      VkPipeline &newPipeline, VkPrimitiveTopology topology) {

    _depth_stencil_create_info = vk_struct_init::get_depth_stencil_create_info(
        true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
    set_vertex_input_state_create_info(vertex_info_description);
    set_input_assembly_create_info(topology);
    // Configure the rasterizer to draw filled triangles
    set_rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    // a single blend attachment with no blending and writing to RGBA
    set_color_blend_attachment_state();
    // we don't use multisampling, so just run the default one
    set_multisampling_state_create_info();
    set_color_blending_info();
    set_viewport_state_info(extent);

    _shaderStages.clear();
    _shaderStages.push_back(shader.get_vertex_shader_stage_info());
    _shaderStages.push_back(shader.get_fragment_shader_stage_info());

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.stageCount = _shaderStages.size();
    pipelineInfo.pStages = _shaderStages.data();
    pipelineInfo.pVertexInputState = &_vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &_inputAssembly;
    pipelineInfo.pViewportState = &_viewportState;
    pipelineInfo.pRasterizationState = &_rasterizer;
    pipelineInfo.pMultisampleState = &_multisampling;
    pipelineInfo.pColorBlendState = &_colorBlending;
    pipelineInfo.pDepthStencilState = &_depth_stencil_create_info;
    pipelineInfo.layout = _pipeline_layout;
    pipelineInfo.renderPass = render_pass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &newPipeline) != VK_SUCCESS) {
      std::cerr << "failed to create pipeline" << std::endl;
      newPipeline = VK_NULL_HANDLE;
    }
  }

  void set_viewport_state_info(VkExtent2D extent) {
    _viewport.x = 0.0f;
    _viewport.y = 0.0f;
    _viewport.width = (float)extent.width;
    _viewport.height = (float)extent.height;
    _viewport.minDepth = 0.0f;
    _viewport.maxDepth = 1.0f;

    _scissor.offset = {0, 0};
    _scissor.extent = extent;

    _viewportState.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    _viewportState.flags = 0;
    _viewportState.pNext = nullptr;

    _viewportState.viewportCount = 1;
    _viewportState.pViewports = &_viewport;
    _viewportState.scissorCount = 1;
    _viewportState.pScissors = &_scissor;
  }

  void set_color_blending_info() {
    // setup dummy color blending. We aren't using transparent objects yet
    // the blending is just "no blend", but we do write to the color attachment
    _colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    _colorBlending.flags = 0;
    _colorBlending.pNext = nullptr;
    _colorBlending.logicOpEnable = VK_FALSE;
    _colorBlending.logicOp = VK_LOGIC_OP_COPY;
    _colorBlending.attachmentCount = 1;
    _colorBlending.pAttachments = &_colorBlendAttachment;
  }

  // Shader inputs for a pipeline
  VkPipelineLayoutCreateInfo
  get_pipeline_layout_create_info(DescriptorSetAllocator &ds_allocator) {
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.pNext = nullptr;

    // empty defaults
    info.flags = 0;
    info.setLayoutCount = ds_allocator.layouts.size();
    info.pSetLayouts = ds_allocator.layouts.data();
    info.pushConstantRangeCount = 0;
    info.pPushConstantRanges = nullptr;
    return info;
  }

  // Vertex buffer and vertex format info
  void set_vertex_input_state_create_info(
      VertexInfoDescription &vertex_info_description) {
    _vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    _vertexInputInfo.pNext = nullptr;

    _vertexInputInfo.vertexBindingDescriptionCount =
        vertex_info_description.bindings.size();
    _vertexInputInfo.pVertexBindingDescriptions =
        vertex_info_description.bindings.data();

    // Fill in the vertex attributes optionally provided by the user
    _vertexInputInfo.vertexAttributeDescriptionCount =
        vertex_info_description.attributes.size();
    _vertexInputInfo.pVertexAttributeDescriptions =
        vertex_info_description.attributes.data();
  }

  // Drawn topology information
  void set_input_assembly_create_info(VkPrimitiveTopology topology) {
    _inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    _inputAssembly.pNext = nullptr;

    _inputAssembly.topology = topology;

    _inputAssembly.primitiveRestartEnable = VK_FALSE;
  }

  void set_rasterization_state_create_info(VkPolygonMode polygonMode) {
    _rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    _rasterizer.flags = 0;
    _rasterizer.pNext = nullptr;

    // Z value of the fragment can be clamped
    _rasterizer.depthClampEnable = VK_FALSE;

    // Discards all primitives before the rasterization stage leaving only
    // vertices
    _rasterizer.rasterizerDiscardEnable = VK_FALSE;

    _rasterizer.polygonMode = polygonMode;
    _rasterizer.lineWidth = 1.0f;
    _rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    _rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    // Depth values of a fragment can be offset by a constant
    _rasterizer.depthBiasEnable = VK_FALSE;
    _rasterizer.depthBiasConstantFactor = 0.0f;
    _rasterizer.depthBiasClamp = 0.0f;
    _rasterizer.depthBiasSlopeFactor = 0.0f;
  }

  // MSAA
  void set_multisampling_state_create_info() {
    _multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    _multisampling.flags = 0;
    _multisampling.pNext = nullptr;

    _multisampling.sampleShadingEnable = VK_FALSE;
    // No MSAA (1 sample per pixel)
    _multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    _multisampling.minSampleShading = 1.0f;
    _multisampling.pSampleMask = nullptr;
    _multisampling.alphaToCoverageEnable = VK_FALSE;
    _multisampling.alphaToOneEnable = VK_FALSE;
  }

  void set_color_blend_attachment_state() {
    _colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    _colorBlendAttachment.blendEnable = VK_FALSE;
  }
};