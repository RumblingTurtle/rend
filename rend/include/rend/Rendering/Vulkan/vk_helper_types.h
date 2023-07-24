#pragma once
#include <cstring>
#include <deque>
#include <filesystem>
#include <functional>
#include <iostream>
#include <rend/Rendering/Vulkan/vk_struct_init.h>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

typedef std::filesystem::path Path;

struct BufferAllocation {
  VkBuffer buffer = VK_NULL_HANDLE;
  VmaAllocation allocation = VK_NULL_HANDLE;
  size_t size;
  VmaAllocator allocator = VK_NULL_HANDLE;

  bool buffer_allocated = false;

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
      throw std::runtime_error("Failed to create buffer");
    }
    buffer_allocation.buffer_allocated = true;
    return buffer_allocation;
  }

  // Single copy
  bool copy_from(void *data, size_t size, size_t offset = 0) {
    void *mapped_data;
    vmaMapMemory(allocator, allocation, &mapped_data);
    memcpy(mapped_data + offset, data, size);
    vmaUnmapMemory(allocator, allocation);
    return true;
  }

  // Multiple copies with different sizes
  // data = [size1, size2, ... , sizeN]
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

  bool buffer_allocated = false;

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
      throw std::runtime_error("ImageAllocation: Failed to create image");
    }
    image_view_create_info.image = image_allocation.image;
    VkResult image_view_result = vkCreateImageView(
        device, &image_view_create_info, nullptr, &image_allocation.view);

    if (image_view_result != VK_SUCCESS) {
      throw std::runtime_error("ImageAllocation: Failed to create image view");
    }

    image_allocation.buffer_allocated = true;
    return image_allocation;
  }

  static ImageAllocation create(VkFormat format, VkImageUsageFlags usage,
                                VkExtent3D extent, VkImageType type,
                                VkImageAspectFlags aspect_flags,
                                VkImageViewType view_type,
                                VmaMemoryUsage vma_usage,
                                VmaAllocationCreateFlags vma_allocation_flags,
                                VkDevice device, VmaAllocator allocator) {

    VkImageCreateInfo image_create_info =
        vk_struct_init::get_image_create_info(format, usage, extent, type);

    VkImageViewCreateInfo image_view_create_info =
        vk_struct_init::get_image_view_create_info(
            nullptr, format, aspect_flags,
            view_type); // Image arg will be substituted from
                        // image_infos image field

    VmaAllocationCreateInfo allocation_info =
        vk_struct_init::get_allocation_info(vma_usage, vma_allocation_flags);

    ImageAllocation image_allocation;
    image_allocation.device = device;
    image_allocation.allocator = allocator;
    image_allocation.format = image_create_info.format;

    VkResult image_result = vmaCreateImage(
        allocator, &image_create_info, &allocation_info,
        &image_allocation.image, &image_allocation.allocation, nullptr);
    if (image_result != VK_SUCCESS) {
      throw std::runtime_error("ImageAllocation: Failed to create image");
    }

    image_view_create_info.image = image_allocation.image;
    VkResult image_view_result = vkCreateImageView(
        device, &image_view_create_info, nullptr, &image_allocation.view);

    if (image_view_result != VK_SUCCESS) {
      throw std::runtime_error("ImageAllocation: Failed to create image view");
    }

    image_allocation.buffer_allocated = true;
    return image_allocation;
  }

  bool valid() { return image != VK_NULL_HANDLE && view != VK_NULL_HANDLE; }

  void destroy() {
    vmaDestroyImage(allocator, image, allocation);
    vkDestroyImageView(device, view, nullptr);
  }
};

// Simple functor queue for deferred deallocation
struct Deallocator {
  std::deque<std::function<void()>> _deletion_queue;
  void push(std::function<void()> &&function) {
    _deletion_queue.push_back(std::move(function));
  }

  void cleanup() {
    while (!_deletion_queue.empty()) {
      _deletion_queue.back()();
      _deletion_queue.pop_back();
    }
  }
};

struct VertexInfoDescription {
  std::vector<VkVertexInputBindingDescription> bindings;
  std::vector<VkVertexInputAttributeDescription> attributes;
  VkPipelineVertexInputStateCreateFlags flags = 0;
};

static std::unordered_map<VkFormat, int> FORMAT_SIZES = {
    {VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float)},
    {VK_FORMAT_R32G32_SFLOAT, 2 * sizeof(float)},
    {VK_FORMAT_R32_SFLOAT, sizeof(float)},
};

inline VertexInfoDescription
get_vertex_info_description(std::vector<VkFormat> attributes,
                            int custom_stride) {
  VertexInfoDescription description;
  VkVertexInputBindingDescription mainBinding = {};
  mainBinding.binding = 0;
  mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  int stride = 0;
  for (int i = 0; i < attributes.size(); i++) {
    VkVertexInputAttributeDescription attr = {};
    attr.binding = 0;
    attr.location = i;
    attr.format = attributes[i];
    attr.offset = stride;
    stride += FORMAT_SIZES[attributes[i]];
    description.attributes.push_back(attr);
  }

  if (custom_stride > 0) {
    mainBinding.stride = custom_stride;
  } else {
    mainBinding.stride = stride;
  }

  description.bindings.push_back(mainBinding);
  return description;
}