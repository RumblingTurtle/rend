#include <rend/Rendering/Vulkan/Material.h>

Material::Material() {}

Material::Material(MaterialSpec spec) : spec(spec) {}

bool Material::build(VkDevice &device, VkDescriptorPool &descriptor_pool,
                     VkRenderPass &render_pass, const VkExtent2D &window_dims,
                     Deallocator &deallocation_queue) {
  ds_allocator.init(spec.bindings, device, descriptor_pool);
  deallocation_queue.push([&]() { ds_allocator.destroy(descriptor_pool); });

  if (spec.vert_shader.native().size() != 0 &&
      spec.frag_shader.native().size() != 0) {
    shader = Shader(spec.vert_shader, spec.frag_shader);
  }
  if (!shader.build_shader_modules(device)) {
    return false;
  }

  _pipeline_builder.build_pipeline_layout(
      device, spec.push_constants_description, ds_allocator, pipeline_layout);

  _pipeline_builder.build_pipeline(
      device, render_pass, shader, pipeline_layout,
      get_vertex_info_description(spec.input_attributes, spec.vertex_stride),
      spec.topology_type, pipeline);

  deallocation_queue.push([=] {
    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
  });

  initialized = true;
  // Shader modules can be destroyed after pipeline creation
  shader.deinit(device);
  return true;
}

void Material::bind_descriptor_buffer(int set_idx, int binding_idx,
                                      BufferAllocation &allocation) {
  if (!allocation.buffer_allocated) {
    throw std::runtime_error("Bound buffer was not allocated ");
  }
  ds_allocator.bind_buffer(set_idx, binding_idx, allocation);
}

void Material::bind_descriptor_texture(int set_idx, int binding_idx,
                                       Texture &texture) {
  if (!texture.image_allocated()) {
    throw std::runtime_error("Bound texture was not allocated ");
  }
  ds_allocator.bind_image(1, 0, texture.image_allocation, texture.layout,
                          texture.sampler);
}
