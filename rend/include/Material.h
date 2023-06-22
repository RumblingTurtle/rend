#pragma once
#include <DescriptorSet.h>
#include <RenderPipelineBuilder.h>
#include <Shader.h>
#include <Texture.h>
#include <memory>

class Material {
  RenderPipelineBuilder _pipeline_builder{};
  bool _pipeline_built = false;
  bool _texture_loaded = false;

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
  VkShaderStageFlags ds_availability =
      VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;

  struct CameraInfo {
    float view[16];
    float projection[16];
  };
  BufferAllocation _camera_buffer;

  struct ModelInfo {
    float model[16];
  };
  BufferAllocation _model_buffer;

  ImageAllocation _texture_image;

  DescriptorSetAllocator ds_allocator;

  Material(Path vert_shader = {}, Path frag_shader = {},
           Path texture_path = {});

  std::vector<VkDescriptorSetLayout> &get_descriptor_layouts() {
    return ds_allocator.layouts;
  }

  bool build_pipeline(VkDevice &device, VkRenderPass &render_pass,
                      VmaAllocator &allocator, VkExtent2D &window_dims,
                      VkDescriptorPool &descriptor_pool,
                      Deallocator &deallocation_queue);

  bool pipeline_built();
};