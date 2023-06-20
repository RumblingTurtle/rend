#pragma once
#include <RenderPipelineBuilder.h>
#include <Shader.h>
#include <memory>

class Material {
  RenderPipelineBuilder _pipeline_builder{};
  bool _pipeline_built = false;

public:
  typedef std::shared_ptr<Material> Ptr;

  Shader shader;
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
    float view[16];
    float projection[16];
  };

  Material(Path vert_shader = {}, Path frag_shader = {});
  bool build_pipeline(VkDevice &device, VkRenderPass &render_pass,
                      VkExtent2D &window_dims, Deallocator &deallocation_queue);
  bool pipeline_built();
};