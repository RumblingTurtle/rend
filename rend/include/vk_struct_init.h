#pragma once
#include <vulkan/vulkan.h>

namespace vk_struct_init {
inline VkImageCreateInfo get_image_create_info(VkFormat format,
                                               VkImageUsageFlags flags,
                                               VkExtent3D extent) {
  VkImageCreateInfo image_info = {};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.pNext = nullptr;
  image_info.flags = 0;
  image_info.imageType = VK_IMAGE_TYPE_2D; // Not interested in 1d or 3d images
  image_info.format = format;
  image_info.extent = extent;
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.usage = flags;
  return image_info;
}

inline VkImageViewCreateInfo
get_image_view_create_info(VkImage image, VkFormat format,
                           VkImageAspectFlags aspectFlags) {
  VkImageViewCreateInfo view_info = {};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.pNext = nullptr;
  view_info.flags = 0;
  view_info.image = image;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = format;

  view_info.subresourceRange.aspectMask = aspectFlags;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = 1;
  return view_info;
}

inline VmaAllocationCreateInfo
get_allocation_info(VmaMemoryUsage usage = VMA_MEMORY_USAGE_GPU_ONLY,
                    VmaAllocationCreateFlags flags = VkMemoryPropertyFlags(
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
  VmaAllocationCreateInfo alloc_info = {};
  alloc_info.usage = usage;
  alloc_info.flags = flags;
  return alloc_info;
}

inline VkPipelineDepthStencilStateCreateInfo
get_depth_stencil_create_info(bool do_depth_test, bool write_depth,
                              VkCompareOp compare_op) {
  VkPipelineDepthStencilStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  info.pNext = nullptr;

  info.depthTestEnable = do_depth_test ? VK_TRUE : VK_FALSE;
  // Write to depth attachment
  info.depthWriteEnable = write_depth ? VK_TRUE : VK_FALSE;
  // Overwrite if the op value is true
  info.depthCompareOp = do_depth_test ? compare_op : VK_COMPARE_OP_ALWAYS;
  info.depthBoundsTestEnable = VK_FALSE;
  info.minDepthBounds = 0.0f;
  info.maxDepthBounds = 1.0f;
  info.stencilTestEnable = VK_FALSE;

  return info;
}

inline VkWriteDescriptorSet get_descriptor_write_info(
    size_t binding, VkDescriptorSet ds, VkDescriptorType type,
    VkDescriptorBufferInfo *buffer_info, VkDescriptorImageInfo *image_info) {
  VkWriteDescriptorSet write = {};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.pNext = nullptr;

  write.dstBinding = binding;
  write.dstSet = ds;
  write.descriptorCount = 1;
  write.descriptorType = type;
  write.pBufferInfo = buffer_info;
  write.pImageInfo = image_info;

  return write;
}

inline VkSamplerCreateInfo
get_sampler_create_info(VkFilter filters, VkSamplerAddressMode addr_mode) {
  VkSamplerCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  info.pNext = nullptr;

  info.addressModeU = addr_mode;
  info.addressModeV = addr_mode;
  info.addressModeW = addr_mode;

  info.magFilter = filters;
  info.minFilter = filters;

  return info;
}
} // namespace vk_struct_init