#include <rend/Rendering/Vulkan/Material.h>

Material::Material(Path vert_shader, Path frag_shader, Path texture_path,
                   std::vector<VkFormat> in_attributes) {
  if (vert_shader.native().size() != 0 && frag_shader.native().size() != 0) {
    shader = Shader(vert_shader, frag_shader);
  }
  texture.load(texture_path);
  vertex_info_description = get_vertex_info_description(in_attributes);

  // Accessible only in the vertex shader
  push_constants_description.offset = 0;
  push_constants_description.size = sizeof(PushConstants);
  push_constants_description.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
}

// Allocates DS buffers and binds them to the descriptor set
bool Material::init(VkDevice &device, VmaAllocator &allocator,
                    VkDescriptorPool &descriptor_pool,
                    Deallocator &deallocation_queue) {
  this->_allocator = allocator;

  // Bindings for the descriptor set
  std::vector<std::vector<Binding>> bindings = {
      {{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sizeof(CameraInfo), 1},
       {VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        sizeof(LightSource), MAX_LIGHTS}} // Camera info (view, projection)
  };

  if (texture.valid) {
    VkSamplerCreateInfo sampler_info =
        vk_struct_init::get_sampler_create_info(filter, address_mode);
    VK_CHECK(vkCreateSampler(device, &sampler_info, nullptr, &sampler),
             "Failed to create sampler");
    texture.allocate_image(device, _allocator, deallocation_queue);
    bindings.push_back({{VK_SHADER_STAGE_FRAGMENT_BIT,
                         VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 1}});
    deallocation_queue.push(
        [&] { vkDestroySampler(device, sampler, nullptr); });
  }

  ds_allocator.init(bindings, device, descriptor_pool);
  deallocation_queue.push([&]() { ds_allocator.destroy(descriptor_pool); });

  // Allocate buffers for the camera and model info and texture
  // This can be generalized to any number of buffers and descriptor sets
  // but will do for now
  _camera_buffer = BufferAllocation::create(
      sizeof(CameraInfo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, _allocator);
  _light_buffer = BufferAllocation::create(
      sizeof(LightSource) * MAX_LIGHTS, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, _allocator);

  deallocation_queue.push([&] {
    _camera_buffer.destroy();
    _light_buffer.destroy();
  });

  // Bind allocated buffers
  ds_allocator.bind_buffer(0, 0, _camera_buffer);
  ds_allocator.bind_buffer(0, 1, _light_buffer);
  if (texture.valid) {
    ds_allocator.bind_image(1, 0, texture.image_allocation,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sampler);
  }
  return true;
}

bool Material::update_lights(std::vector<LightSource> &lights) {
  if (lights.size() > MAX_LIGHTS) {
    std::cerr << "Light count exceeds maximum allowed lights" << std::endl;
    return false;
  }
  _light_buffer.copy_from(lights.data(), sizeof(LightSource) * lights.size());
  return true;
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
                                          ds_allocator, pipeline_layout);
  _pipeline_builder.build_pipeline(device, render_pass, window_dims, shader,
                                   pipeline_layout, vertex_info_description,
                                   pipeline, topology_type);

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