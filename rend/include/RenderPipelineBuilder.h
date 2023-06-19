#pragma once
#include <Shader.h>
#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>

class RenderPipelineBuilder {
public:
  std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
  VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
  VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
  VkViewport _viewport;
  VkRect2D _scissor;
  VkPipelineRasterizationStateCreateInfo _rasterizer;
  VkPipelineColorBlendAttachmentState _colorBlendAttachment;
  VkPipelineMultisampleStateCreateInfo _multisampling;
  VkPipelineLayout _pipelineLayout;

  VkPipeline build_pipeline(VkDevice device, VkRenderPass render_pass,
                            Shader &shader, VkExtent2D extent,
                            VkPipelineLayout _pipeline_layout) {

    _shaderStages.push_back(shader.get_vertex_shader_stage_info());
    _shaderStages.push_back(shader.get_fragment_shader_stage_info());
    _vertexInputInfo = get_vertex_input_state_create_info();
    _inputAssembly =
        get_input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    _viewport.x = 0.0f;
    _viewport.y = 0.0f;
    _viewport.width = (float)extent.width;
    _viewport.height = (float)extent.height;
    _viewport.minDepth = 0.0f;
    _viewport.maxDepth = 1.0f;
    _scissor.offset = {0, 0};
    _scissor.extent = extent;

    // Configure the rasterizer to draw filled triangles
    _rasterizer = get_rasterization_state_create_info(VK_POLYGON_MODE_FILL);

    // we don't use multisampling, so just run the default one
    _multisampling = get_multisampling_state_create_info();

    // a single blend attachment with no blending and writing to RGBA
    _colorBlendAttachment = get_color_blend_attachment_state();

    // use the triangle layout we created
    this->_pipelineLayout = _pipeline_layout;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = nullptr;

    viewportState.viewportCount = 1;
    viewportState.pViewports = &_viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &_scissor;

    // setup dummy color blending. We aren't using transparent objects yet
    // the blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = nullptr;

    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &_colorBlendAttachment;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = nullptr;

    pipelineInfo.stageCount = _shaderStages.size();
    pipelineInfo.pStages = _shaderStages.data();
    pipelineInfo.pVertexInputState = &_vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &_inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &_rasterizer;
    pipelineInfo.pMultisampleState = &_multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = this->_pipelineLayout;
    pipelineInfo.renderPass = render_pass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    // it's easy to error out on create graphics pipeline, so we handle it a bit
    // better than the common VK_CHECK case
    VkPipeline newPipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &newPipeline) != VK_SUCCESS) {
      std::cerr << "failed to create pipeline" << std::endl;
      return VK_NULL_HANDLE; // failed to create graphics pipeline
    } else {
      return newPipeline;
    }
  }

  // Shader inputs for a pipeline
  VkPipelineLayoutCreateInfo get_pipeline_layout_create_info() {
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.pNext = nullptr;

    // empty defaults
    info.flags = 0;
    info.setLayoutCount = 0;
    info.pSetLayouts = nullptr;
    info.pushConstantRangeCount = 0;
    info.pPushConstantRanges = nullptr;
    return info;
  }

  // Vertex buffer and vertex format info
  VkPipelineVertexInputStateCreateInfo get_vertex_input_state_create_info() {
    VkPipelineVertexInputStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    info.pNext = nullptr;

    // no vertex bindings or attributes
    info.vertexBindingDescriptionCount = 0;
    info.vertexAttributeDescriptionCount = 0;
    return info;
  }

  // Drawn topology information
  VkPipelineInputAssemblyStateCreateInfo
  get_input_assembly_create_info(VkPrimitiveTopology topology) {
    VkPipelineInputAssemblyStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    info.pNext = nullptr;

    info.topology = topology;

    info.primitiveRestartEnable = VK_FALSE;
    return info;
  }

  VkPipelineRasterizationStateCreateInfo
  get_rasterization_state_create_info(VkPolygonMode polygonMode) {
    VkPipelineRasterizationStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    info.pNext = nullptr;

    // Z value of the fragment can be clamped
    info.depthClampEnable = VK_FALSE;

    // Discards all primitives before the rasterization stage leaving only
    // vertices
    info.rasterizerDiscardEnable = VK_FALSE;

    info.polygonMode = polygonMode;
    info.lineWidth = 1.0f;
    // No backface culling
    info.cullMode = VK_CULL_MODE_NONE;
    info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    // Depth values of a fragment can be offset by a constant
    info.depthBiasEnable = VK_FALSE;
    info.depthBiasConstantFactor = 0.0f;
    info.depthBiasClamp = 0.0f;
    info.depthBiasSlopeFactor = 0.0f;

    return info;
  }

  // MSAA
  VkPipelineMultisampleStateCreateInfo get_multisampling_state_create_info() {
    VkPipelineMultisampleStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    info.pNext = nullptr;

    info.sampleShadingEnable = VK_FALSE;
    // No MSAA (1 sample per pixel)
    info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    info.minSampleShading = 1.0f;
    info.pSampleMask = nullptr;
    info.alphaToCoverageEnable = VK_FALSE;
    info.alphaToOneEnable = VK_FALSE;
    return info;
  }

  VkPipelineColorBlendAttachmentState get_color_blend_attachment_state() {
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    return colorBlendAttachment;
  }
};