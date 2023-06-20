#pragma once
#include <filesystem>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#define VMA_IMPLEMENTATION

typedef std::filesystem::path Path;

struct VertexInfoDescription {
  std::vector<VkVertexInputBindingDescription> bindings;
  std::vector<VkVertexInputAttributeDescription> attributes;
  VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct AllocatedBuffer {
  VkBuffer _buffer;
  VmaAllocation _allocation;
};