#pragma once
#include <filesystem>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

typedef std::filesystem::path Path;

struct VertexInfoDescription {
  std::vector<VkVertexInputBindingDescription> bindings;
  std::vector<VkVertexInputAttributeDescription> attributes;

  VkPipelineVertexInputStateCreateFlags flags = 0;
};

#define VMA_IMPLEMENTATION
struct VertexInfo {
  float position[3];
  float normal[3];
  float uv[2];

  static VertexInfoDescription get_vertex_info_description() {
    VertexInfoDescription description;
    VkVertexInputBindingDescription mainBinding = {};
    mainBinding.binding = 0;
    mainBinding.stride = sizeof(VertexInfo);
    mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    description.bindings.push_back(mainBinding);

    // Position will be stored at Location 0
    VkVertexInputAttributeDescription position_attr = {};
    position_attr.binding = 0;
    position_attr.location = 0;
    position_attr.format = VK_FORMAT_R32G32B32_SFLOAT;
    position_attr.offset = offsetof(VertexInfo, position);

    // Normal will be stored at Location 1
    VkVertexInputAttributeDescription normal_attr = {};
    normal_attr.binding = 0;
    normal_attr.location = 1;
    normal_attr.format = VK_FORMAT_R32G32B32_SFLOAT;
    normal_attr.offset = offsetof(VertexInfo, normal);

    // UV coords will be stored at Location 2
    VkVertexInputAttributeDescription uv_attr = {};
    uv_attr.binding = 0;
    uv_attr.location = 2;
    uv_attr.format = VK_FORMAT_R32G32_SFLOAT;
    uv_attr.offset = offsetof(VertexInfo, uv);

    description.attributes.push_back(position_attr);
    description.attributes.push_back(normal_attr);
    description.attributes.push_back(uv_attr);
    return description;
  }
};

struct AllocatedBuffer {
  VkBuffer _buffer;
  VmaAllocation _allocation;
};