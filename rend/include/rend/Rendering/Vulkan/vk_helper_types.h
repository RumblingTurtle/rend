#pragma once
#include <cstring>
#include <deque>
#include <filesystem>
#include <functional>
#include <iostream>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

typedef std::filesystem::path Path;

struct BufferAllocation {
  VkBuffer buffer = VK_NULL_HANDLE;
  VmaAllocation allocation = VK_NULL_HANDLE;
  size_t size;
  VmaAllocator allocator = VK_NULL_HANDLE;

  static BufferAllocation create(size_t size, VkBufferUsageFlags buffer_usage,
                                 VmaMemoryUsage alloc_usage,
                                 VmaAllocator allocator) {

    BufferAllocation buffer_allocation;
    buffer_allocation.size = size;
    buffer_allocation.allocator = allocator;

    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = buffer_usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo vmaalloc_info = {};
    vmaalloc_info.usage = alloc_usage;

    if (vmaCreateBuffer(allocator, &buffer_info, &vmaalloc_info,
                        &buffer_allocation.buffer,
                        &buffer_allocation.allocation, nullptr) != VK_SUCCESS) {
      return {};
    }

    return buffer_allocation;
  }

  // Single copy
  bool copy_from(void *data, size_t size) {
    void *mapped_data;
    vmaMapMemory(allocator, allocation, &mapped_data);
    memcpy(mapped_data, data, size);
    vmaUnmapMemory(allocator, allocation);
    return true;
  }

  // Multiple copies with different sizes
  bool copy_from(void *datas[], size_t sizes[], size_t num_datas) {
    void *mapped_data;
    vmaMapMemory(allocator, allocation, &mapped_data);
    int offset = 0;
    for (int i = 0; i < num_datas; i++) {
      memcpy(mapped_data + offset, datas[i], sizes[i]); // It's fine:)
      offset += sizes[i];
    }
    vmaUnmapMemory(allocator, allocation);
    return true;
  }

  bool valid() { return buffer != VK_NULL_HANDLE; }

  void destroy() { vmaDestroyBuffer(allocator, buffer, allocation); }
};

struct ImageAllocation {
  VkImage image = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
  VmaAllocation allocation = VK_NULL_HANDLE;
  VkFormat format;

  // Ptrs from the renderer
  VmaAllocator allocator;
  VkDevice device;

  static ImageAllocation create(VkImageCreateInfo &image_create_info,
                                VkImageViewCreateInfo &image_view_create_info,
                                VmaAllocationCreateInfo &allocation_info,
                                VkDevice device, VmaAllocator allocator) {
    ImageAllocation image_allocation;
    image_allocation.device = device;
    image_allocation.allocator = allocator;
    image_allocation.format = image_create_info.format;

    VkResult image_result = vmaCreateImage(
        allocator, &image_create_info, &allocation_info,
        &image_allocation.image, &image_allocation.allocation, nullptr);
    if (image_result != VK_SUCCESS) {
      std::cerr << "ImageAllocation: Failed to create image" << std::endl;
      return {};
    }
    image_view_create_info.image = image_allocation.image;
    VkResult image_view_result = vkCreateImageView(
        device, &image_view_create_info, nullptr, &image_allocation.view);

    if (image_view_result != VK_SUCCESS) {

      std::cerr << "ImageAllocation: Failed to create image view" << std::endl;
      return {};
    }
    return image_allocation;
  }

  bool valid() { return image != VK_NULL_HANDLE && view != VK_NULL_HANDLE; }

  void destroy() {
    vmaDestroyImage(allocator, image, allocation);
    vkDestroyImageView(device, view, nullptr);
  }
};

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
