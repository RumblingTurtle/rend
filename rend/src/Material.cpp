#include <Material.h>

Material::Material(Path vert_shader, Path frag_shader, Path texture_path) {
  if (vert_shader.native().size() != 0 && frag_shader.native().size() != 0) {
    shader = Shader(vert_shader, frag_shader);
  }

  if (texture_path.native().size() != 0) {
    _texture_loaded = texture.load(texture_path);
  } else {
    _texture_loaded = texture.load(
        Path(ASSET_DIRECTORY + std::string{"/textures/error.jpg"}));
  }

  vertex_info_description = VertexShaderInput::get_vertex_info_description();

  // Accessible only in the vertex shader
  push_constants_description.offset = 0;
  push_constants_description.size = sizeof(PushConstants);
  push_constants_description.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
}

bool Material::build_pipeline(VkDevice &device, VkRenderPass &render_pass,
                              VmaAllocator &allocator, VkExtent2D &window_dims,
                              VkDescriptorPool &descriptor_pool,
                              Deallocator &deallocation_queue) {
  if (_pipeline_built) {
    return true;
  }

  if (_texture_loaded) {
    texture.allocate_image(device, allocator, deallocation_queue);
  }

  if (!shader.build_shader_modules(device)) {
    return false;
  }

  {
    // Build sampler
    VkSamplerCreateInfo sampler_info =
        vk_struct_init::get_sampler_create_info(filter, address_mode);
    VK_CHECK(vkCreateSampler(device, &sampler_info, nullptr, &sampler),
             "Failed to create sampler");
    // Bindings for the descriptor set
    std::vector<std::vector<Binding>> bindings = {
        {{ds_availability, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          sizeof(CameraInfo)}}, // Camera info (view, projection)
        {{ds_availability, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
          texture.get_pixels_size()}}, // Texture
        {{ds_availability, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          sizeof(ModelInfo)}}}; // Model info

    ds_allocator.assemble_layouts(bindings, device);
    ds_allocator.allocate_descriptor_sets(descriptor_pool);

    // Allocate buffers for the camera and model info and texture
    _camera_buffer = BufferAllocation::create(
        sizeof(CameraInfo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU, allocator);
    _model_buffer = BufferAllocation::create(
        sizeof(ModelInfo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU, allocator);

    ds_allocator.bind_buffer(0, 0, _camera_buffer);
    ds_allocator.bind_image(1, 0, texture.image_allocation,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler);
    ds_allocator.bind_buffer(2, 0, _model_buffer);
    deallocation_queue.push([=] {
      vkDestroySampler(device, sampler, nullptr);
      _camera_buffer.destroy();
      _model_buffer.destroy();
      ds_allocator.destroy(descriptor_pool);
    });
  }

  _pipeline_builder.build_pipeline_layout(device, push_constants_description,
                                          ds_allocator, pipeline_layout);
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