#pragma once
#include <fstream>
#include <macros.h>
#include <string>
#include <types.h>
#include <vector>
#include <vulkan/vulkan.h>

class Shader {
  Path _fragment_file_path;
  Path _vertex_file_path;

  VkShaderModule _vertex_module;
  VkShaderModule _fragment_module;

  std::vector<uint32_t> _vertex_code;
  std::vector<uint32_t> _fragment_code;

  bool _built_modules = false;
  bool _default_shader = true; // True if shader code was loaded

  // Generic shader module creation
  VkPipelineShaderStageCreateInfo
  get_shader_stage_info(VkShaderStageFlagBits stage,
                        VkShaderModule shaderModule);

public:
  Shader(Path vertex_path, Path fragment_path) {
    init(vertex_path, fragment_path);
    _default_shader = false;
  }

  Shader() {
    init(ASSET_DIRECTORY + std::string{"/shaders/bin/default_mesh_vert.spv"},
         ASSET_DIRECTORY + std::string{"/shaders/bin/default_mesh_frag.spv"});
  }

  // Load shader code from file
  bool init(Path vertex_path, Path fragment_path);

  // Destroy shader modules
  void deinit(VkDevice &_device);

  bool default_shader() { return _default_shader; }

  bool build_shader_modules(VkDevice &_device);

  VkPipelineShaderStageCreateInfo get_vertex_shader_stage_info();

  VkPipelineShaderStageCreateInfo get_fragment_shader_stage_info();
};