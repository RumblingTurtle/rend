#pragma once

#include <memory>
#include <rend/LightSource.h>
#include <rend/Rendering/Vulkan/DescriptorSet.h>
#include <rend/Rendering/Vulkan/RenderPipelineBuilder.h>
#include <rend/Rendering/Vulkan/Shader.h>
#include <rend/Rendering/Vulkan/Texture.h>
#include <unordered_map>

constexpr int MAX_LIGHTS = 64;

struct PushConstants {
  float model[16];
  int texture_idx;
  int light_index;
};

struct CameraInfo {
  float view[16];
  float projection[16];
  float position[4];
};

constexpr int MAX_TEXTURE_COUNT = 20;
static BindingMatrix DEFAULT_BINDINGS = {
    {Binding{VK_SHADER_STAGE_FRAGMENT_BIT,              //
             VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, //
             0,                                         //
             MAX_TEXTURE_COUNT}},                       // Textures
    {Binding{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, //
             VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,                         //
             sizeof(CameraInfo),                                        //
             1},
     Binding{VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, //
             VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,                         //
             sizeof(LightSource),                                       //
             MAX_LIGHTS},                  // Camera info (view, projection)
     Binding{VK_SHADER_STAGE_FRAGMENT_BIT, //
             VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, //
             0,                                         //
             1}}};                                      // Shadow map

static std::vector<VkFormat> DEFAULT_INPUT_LAYOUT = {
    VK_FORMAT_R32G32B32_SFLOAT, // Position
    VK_FORMAT_R32G32B32_SFLOAT, // Normal
    VK_FORMAT_R32G32_SFLOAT};   // UV

struct MaterialSpec {
  Path vert_shader;
  Path frag_shader;
  BindingMatrix bindings;
  VkPrimitiveTopology topology_type = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  int vertex_stride = -1;
  std::vector<VkFormat> input_attributes = DEFAULT_INPUT_LAYOUT;

  int color_attachment_count;
  bool depth_test_enabled;
  bool blend_test_enabled;

  VkPushConstantRange push_constants_description{
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, //
      0,                                                         //
      sizeof(PushConstants)                                      //
  };
};

class Material {
  RenderPipelineBuilder _pipeline_builder{};
  VmaAllocator _allocator = VK_NULL_HANDLE;

public:
  typedef std::shared_ptr<Material> Ptr;
  typedef std::unique_ptr<Material> UPtr;

  bool initialized = false;

  MaterialSpec spec;

  Shader shader;

  VkPipelineLayout pipeline_layout;
  DescriptorSetAllocator ds_allocator;

  VkPipeline pipeline;

  Material();

  Material(MaterialSpec spec);

  // Initialization
  bool build(VkDevice &device, VkDescriptorPool &descriptor_pool,
             VkRenderPass &render_pass, const VkExtent2D &window_dims,
             Deallocator &deallocation_queue);

  // Descriptor binding
  void bind_descriptor_buffer(int set_idx, int binding_idx,
                              BufferAllocation &allocation);
  void bind_descriptor_texture(int set_idx, int binding_idx, Texture &texture);
};