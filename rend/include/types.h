#pragma once
#include <deque>
#include <filesystem>
#include <functional>
#include <vector>
#include <vulkan/vulkan.h>

typedef std::filesystem::path Path;

struct VertexInfoDescription {
  std::vector<VkVertexInputBindingDescription> bindings;
  std::vector<VkVertexInputAttributeDescription> attributes;
  VkPipelineVertexInputStateCreateFlags flags = 0;
};

// Simple functor queue for deferred deallocation
struct Deallocator {
  std::deque<std::function<void()>> _deletion_queue;
  void push(std::function<void()> &&function) {
    _deletion_queue.push_back(std::move(function));
  }

  ~Deallocator() {
    while (!_deletion_queue.empty()) {
      _deletion_queue.back()();
      _deletion_queue.pop_back();
    }
  }
};
