#pragma once
#include <DescriptorSet.h>
#include <LightSource.h>
#include <RenderPipelineBuilder.h>
#include <Shader.h>
#include <Texture.h>
#include <constants.h>
#include <memory>

class Material {
  RenderPipelineBuilder _pipeline_builder{};
  bool _pipeline_built = false;
  bool _texture_loaded = false;

  VmaAllocator _allocator = VK_NULL_HANDLE;

public:
  typedef std::shared_ptr<Material> Ptr;

  Shader shader;
  Texture texture;

  VkFilter filter = VK_FILTER_LINEAR;
  VkSamplerAddressMode address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  VkSampler sampler;

  VertexInfoDescription vertex_info_description;
  VkPushConstantRange push_constants_description;

  VkPipeline pipeline;
  VkPipelineLayout pipeline_layout;

  struct VertexShaderInput {
    float position[3];
    float normal[3];
    float uv[2];

    static VertexInfoDescription get_vertex_info_description();
  };

  struct PushConstants {
    float model[16];
  };

  struct CameraInfo {
    float view[16];
    float projection[16];
    float position[4];
  };
  BufferAllocation _camera_buffer;

  struct ModelInfo {
    float model[16];
  };

  ImageAllocation _texture_image;
  BufferAllocation _light_buffer;
  BufferAllocation _model_buffer;

  DescriptorSetAllocator ds_allocator;

  Material(Path vert_shader = {}, Path frag_shader = {},
           Path texture_path = {});

  std::vector<VkDescriptorSetLayout> &get_descriptor_layouts() {
    return ds_allocator.layouts;
  }

  bool bind_allocator(VmaAllocator allocator) {
    if (allocator == VK_NULL_HANDLE) {
      return false;
    }
    _allocator = allocator;
    return true;
  }

  bool allocate_texture(VkDevice &device, Deallocator &deallocation_queue);

  bool init_descriptor_sets(VkDevice &device, VkDescriptorPool &descriptor_pool,
                            Deallocator &deallocation_queue);

  bool update_lights(std::vector<LightSource> &lights);

  bool bind_buffers_and_images();

  bool build_pipeline(VkDevice &device, VkRenderPass &render_pass,
                      VkExtent2D &window_dims, Deallocator &deallocation_queue);

  bool pipeline_built();
};