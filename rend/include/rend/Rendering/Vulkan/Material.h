#pragma once

#include <memory>
#include <rend/LightSource.h>
#include <rend/Rendering/Vulkan/DescriptorSet.h>
#include <rend/Rendering/Vulkan/RenderPipelineBuilder.h>
#include <rend/Rendering/Vulkan/Shader.h>
#include <rend/Rendering/Vulkan/Texture.h>
#include <unordered_map>

constexpr int MAX_LIGHTS = 8;

struct PushConstants {
  float model[16];
};

struct CameraInfo {
  float view[16];
  float projection[16];
  float position[4];
};

struct ModelInfo {
  float model[16];
};

class Material {
  RenderPipelineBuilder _pipeline_builder{};
  bool _pipeline_built = false;

  VmaAllocator _allocator = VK_NULL_HANDLE;

public:
  typedef std::shared_ptr<Material> Ptr;

  VkPrimitiveTopology topology_type = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

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
  };

  struct DebugShaderInput {
    float position[3];
  };

  BufferAllocation _camera_buffer;
  BufferAllocation _light_buffer;

  DescriptorSetAllocator ds_allocator;

  Material(Path vert_shader = {}, Path frag_shader = {}, Path texture_path = {},
           std::vector<VkFormat> in_attributes = {});

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

  bool init(VkDevice &device, VmaAllocator &allocator,
            VkDescriptorPool &descriptor_pool, Deallocator &deallocation_queue);

  bool update_lights(std::vector<LightSource> &lights);

  bool build_pipeline(VkDevice &device, VkRenderPass &render_pass,
                      VkExtent2D &window_dims, Deallocator &deallocation_queue);

  bool pipeline_built();
};