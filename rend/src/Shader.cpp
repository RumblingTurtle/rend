#include <Shader.h>

VkPipelineShaderStageCreateInfo
Shader::get_shader_stage_info(VkShaderStageFlagBits stage,
                              VkShaderModule shaderModule) {
  VkPipelineShaderStageCreateInfo info{};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  info.pNext = nullptr;

  // shader stage
  info.stage = stage;
  // module containing the code for this shader stage
  info.module = shaderModule;
  // the entry point of the shader
  info.pName = "main";
  return info;
}

bool Shader::init(Path vertex_path, Path fragment_path) {
  _vertex_file_path = vertex_path;
  _fragment_file_path = fragment_path;

  std::ifstream vertex_file;
  std::ifstream fragment_file;

  fragment_file.open(_fragment_file_path, std::ios::ate | std::ios::binary);
  vertex_file.open(_vertex_file_path, std::ios::ate | std::ios::binary);
  if (!vertex_file.is_open() || !fragment_file.is_open()) {
    std::cerr << "Failed to open shader files" << std::endl
              << _fragment_file_path << std::endl
              << _vertex_file_path << std::endl;
    return false;
  }

  size_t vert_file_size = vertex_file.tellg();
  size_t frag_file_size = fragment_file.tellg();

  _vertex_code.resize(vert_file_size / sizeof(uint32_t));
  _fragment_code.resize(frag_file_size / sizeof(uint32_t));

  vertex_file.seekg(0);
  vertex_file.read((char *)_vertex_code.data(), vert_file_size);
  vertex_file.close();

  fragment_file.seekg(0);
  fragment_file.read((char *)_fragment_code.data(), frag_file_size);
  fragment_file.close();
  return true;
}

bool Shader::build_shader_modules(VkDevice &_device) {
  _built_modules = true;
  VkShaderModuleCreateInfo vertex_info{};
  vertex_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  vertex_info.codeSize = _vertex_code.size() * sizeof(uint32_t);
  vertex_info.pCode = _vertex_code.data();

  VkShaderModuleCreateInfo fragment_info{};
  fragment_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  fragment_info.codeSize = _fragment_code.size() * sizeof(uint32_t);
  fragment_info.pCode = _fragment_code.data();

  VK_CHECK(
      vkCreateShaderModule(_device, &vertex_info, nullptr, &_vertex_module),
      "Failed to create vertex shader module");
  VK_CHECK(
      vkCreateShaderModule(_device, &fragment_info, nullptr, &_fragment_module),
      "Failed to create fragment shader module");
  return true;
}

void Shader::deinit(VkDevice &_device) {
  if (_built_modules) {
    vkDestroyShaderModule(_device, _vertex_module, nullptr);
    vkDestroyShaderModule(_device, _fragment_module, nullptr);
  }
}

VkPipelineShaderStageCreateInfo Shader::get_vertex_shader_stage_info() {
  if (!_built_modules) {
    throw std::runtime_error("Shader not initialized");
  }
  return get_shader_stage_info(VK_SHADER_STAGE_VERTEX_BIT, _vertex_module);
}

VkPipelineShaderStageCreateInfo Shader::get_fragment_shader_stage_info() {
  if (!_built_modules) {
    throw std::runtime_error("Shader not initialized");
  }
  return get_shader_stage_info(VK_SHADER_STAGE_FRAGMENT_BIT, _fragment_module);
}
