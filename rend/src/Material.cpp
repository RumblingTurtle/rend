#include <Material.h>

Material::Material(Path vert_shader, Path frag_shader) {
  if (vert_shader.native().size() != 0 && frag_shader.native().size() != 0) {
    shader = Shader(vert_shader, frag_shader);
  }

  vertex_info_description = VertexShaderInput::get_vertex_info_description();

  // Accessible only in the vertex shader
  push_constants_description.offset = 0;
  push_constants_description.size = sizeof(PushConstants);
  push_constants_description.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
}

bool Material::build_pipeline(VkDevice &device, VkRenderPass &render_pass,
                              VkExtent2D &window_dims,
                              Deallocator &deallocation_queue) {
  if (_pipeline_built) {
    return true;
  }
  if (!shader.build_shader_modules(device)) {
    return false;
  }

  _pipeline_builder.build_pipeline_layout(device, push_constants_description,
                                          pipeline_layout);
  _pipeline_builder.build_pipeline(device, render_pass, window_dims, shader,
                                   pipeline_layout, vertex_info_description,
                                   pipeline);

  // Shader modules can be destroyed after pipeline creation
  shader.deinit(device);

  deallocation_queue.push([=] {
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
  });

  _pipeline_built = true;
  return true;
}

bool Material::pipeline_built() { return _pipeline_built; }

VertexInfoDescription
Material::VertexShaderInput::get_vertex_info_description() {
  VertexInfoDescription description;
  VkVertexInputBindingDescription mainBinding = {};
  mainBinding.binding = 0;
  mainBinding.stride = sizeof(VertexShaderInput);
  mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  description.bindings.push_back(mainBinding);

  // Position will be stored at Location 0
  VkVertexInputAttributeDescription position_attr = {};
  position_attr.binding = 0;
  position_attr.location = 0;
  position_attr.format = VK_FORMAT_R32G32B32_SFLOAT;
  position_attr.offset = offsetof(VertexShaderInput, position);

  // Normal will be stored at Location 1
  VkVertexInputAttributeDescription normal_attr = {};
  normal_attr.binding = 0;
  normal_attr.location = 1;
  normal_attr.format = VK_FORMAT_R32G32B32_SFLOAT;
  normal_attr.offset = offsetof(VertexShaderInput, normal);

  // UV coords will be stored at Location 2
  VkVertexInputAttributeDescription uv_attr = {};
  uv_attr.binding = 0;
  uv_attr.location = 2;
  uv_attr.format = VK_FORMAT_R32G32_SFLOAT;
  uv_attr.offset = offsetof(VertexShaderInput, uv);

  description.attributes.push_back(position_attr);
  description.attributes.push_back(normal_attr);
  description.attributes.push_back(uv_attr);
  return description;
}