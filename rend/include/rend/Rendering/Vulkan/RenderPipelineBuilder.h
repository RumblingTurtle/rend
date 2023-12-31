#pragma once
#include <iostream>
#include <rend/Rendering/Vulkan/DescriptorSet.h>
#include <rend/Rendering/Vulkan/Shader.h>
#include <rend/Rendering/Vulkan/vk_struct_init.h>
#include <rend/macros.h>
#include <vector>
#include <vulkan/vulkan.h>

class RenderPipelineBuilder {
  VertexInfoDescription _vertex_info_description{};
  std::vector<VkPipelineShaderStageCreateInfo> _shader_stages{};
  VkPipelineVertexInputStateCreateInfo _vertex_input_info{};
  VkPipelineInputAssemblyStateCreateInfo _input_assembly{};
  VkPipelineRasterizationStateCreateInfo _rasterizer{};
  std::vector<VkPipelineColorBlendAttachmentState> _color_blend_attachments;
  VkPipelineMultisampleStateCreateInfo _multisampling{};
  VkPipelineColorBlendStateCreateInfo _color_blending{};
  VkPipelineViewportStateCreateInfo _viewport_state{};
  VkPipelineDepthStencilStateCreateInfo _depth_stencil_create_info{};
  VkPipelineLayoutCreateInfo pipeline_layout_info{};

public:
  void build_pipeline_layout(VkDevice &_device,
                             VkPushConstantRange &push_constant_range,
                             DescriptorSetAllocator &ds_allocator,
                             VkPipelineLayout &layout_out) {
    pipeline_layout_info = get_pipeline_layout_create_info(ds_allocator);

    pipeline_layout_info.pPushConstantRanges = &push_constant_range;
    pipeline_layout_info.pushConstantRangeCount = 1;

    if (vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr,
                               &layout_out) != VK_SUCCESS) {
      std::cerr << "Failed to create pipeline layout" << std::endl;
    }
  }

  void build_pipeline(VkDevice &device, VkRenderPass &render_pass,
                      Shader &shader, VkPipelineLayout &pipeline_layout,
                      VertexInfoDescription vertex_info_description,
                      VkPrimitiveTopology topology, VkPipeline &new_pipeline,
                      int color_attachment_count, bool depth_test_enabled,
                      bool blend_test_enabled) {
    _vertex_info_description = vertex_info_description;
    _depth_stencil_create_info = vk_struct_init::get_depth_stencil_create_info(
        depth_test_enabled, depth_test_enabled, VK_COMPARE_OP_LESS_OR_EQUAL);
    set_vertex_input_state_create_info();
    set_input_assembly_create_info(topology);
    // Configure the rasterizer to draw filled triangles
    set_rasterization_state_create_info(VK_POLYGON_MODE_FILL);
    // a single blend attachment with no blending and writing to RGBA
    // we don't use multisampling, so just run the default one
    set_multisampling_state_create_info();

    set_color_blending_info(color_attachment_count, blend_test_enabled);

    VkDynamicState dynamicState[2] = {VK_DYNAMIC_STATE_VIEWPORT,
                                      VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
    dynamicStateCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = 2;
    dynamicStateCreateInfo.pDynamicStates = dynamicState;

    _viewport_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    _viewport_state.pViewports = nullptr;
    _viewport_state.viewportCount = 1;
    _viewport_state.scissorCount = 1;

    _shader_stages.clear();
    _shader_stages.push_back(shader.get_vertex_shader_stage_info());
    _shader_stages.push_back(shader.get_fragment_shader_stage_info());

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;
    pipelineInfo.stageCount = _shader_stages.size();
    pipelineInfo.pStages = _shader_stages.data();
    pipelineInfo.pVertexInputState = &_vertex_input_info;
    pipelineInfo.pInputAssemblyState = &_input_assembly;
    pipelineInfo.pRasterizationState = &_rasterizer;
    pipelineInfo.pMultisampleState = &_multisampling;
    pipelineInfo.pColorBlendState = &_color_blending;
    pipelineInfo.pDepthStencilState = &_depth_stencil_create_info;
    pipelineInfo.layout = pipeline_layout;
    pipelineInfo.renderPass = render_pass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.pViewportState = &_viewport_state;
    pipelineInfo.pDynamicState = &dynamicStateCreateInfo;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &new_pipeline) != VK_SUCCESS) {
      std::cerr << "failed to create pipeline" << std::endl;
      new_pipeline = VK_NULL_HANDLE;
    }
  }

  void set_color_blending_info(int color_attachment_count, bool blend_enabled) {
    _color_blend_attachments.resize(color_attachment_count);
    for (auto &attachment : _color_blend_attachments) {
      attachment.colorWriteMask =
          VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
      attachment.blendEnable = blend_enabled ? VK_TRUE : VK_FALSE;

      attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
      attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
      attachment.colorBlendOp = VK_BLEND_OP_ADD;
      attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
      attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    }

    // setup dummy color blending. We aren't using transparent objects yet
    // the blending is just "no blend", but we do write to the color attachment
    _color_blending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    _color_blending.flags = 0;
    _color_blending.pNext = nullptr;
    _color_blending.logicOpEnable = VK_FALSE;
    _color_blending.logicOp = VK_LOGIC_OP_COPY;
    _color_blending.attachmentCount = _color_blend_attachments.size();
    _color_blending.pAttachments = _color_blend_attachments.data();
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
  void set_vertex_input_state_create_info() {
    _vertex_input_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    _vertex_input_info.pNext = nullptr;

    _vertex_input_info.vertexBindingDescriptionCount =
        _vertex_info_description.bindings.size();
    _vertex_input_info.pVertexBindingDescriptions =
        _vertex_info_description.bindings.data();

    // Fill in the vertex attributes optionally provided by the user
    _vertex_input_info.vertexAttributeDescriptionCount =
        _vertex_info_description.attributes.size();
    _vertex_input_info.pVertexAttributeDescriptions =
        _vertex_info_description.attributes.data();
  }

  // Drawn topology information
  void set_input_assembly_create_info(VkPrimitiveTopology topology) {
    _input_assembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    _input_assembly.pNext = nullptr;

    _input_assembly.topology = topology;

    _input_assembly.primitiveRestartEnable = VK_FALSE;
  }

  void set_rasterization_state_create_info(VkPolygonMode polygonMode) {
    _rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    _rasterizer.flags = 0;
    _rasterizer.pNext = nullptr;

    _rasterizer.rasterizerDiscardEnable = VK_FALSE;

    _rasterizer.polygonMode = polygonMode;
    _rasterizer.lineWidth = 1.0f;
    _rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    _rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    _rasterizer.depthClampEnable = VK_FALSE;
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
};