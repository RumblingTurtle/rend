#pragma once
#include <vulkan/vulkan.h>

/**
 * Helper functions for initializing vulkan structs
 * This should be the first place to look when you start to have issues
 * Filler functions have to:
 *  1)Fill all the fileds of the struct
 *  2)USE NO HARDCODED DEFAULTS (although currently there are some which will be
 * removed )
 */
namespace vk_struct_init {
inline VkImageCreateInfo get_image_create_info(VkFormat format,
                                               VkImageUsageFlags usage,
                                               VkExtent3D extent,
                                               VkImageType type) {
  /*
  typedef struct VkImageCreateInfo {
    VkStructureType          sType; +
    const void*              pNext; +
    VkImageCreateFlags       flags;
    VkImageType              imageType;
    VkFormat                 format; +
    VkExtent3D               extent; +
    uint32_t                 mipLevels;
    uint32_t                 arrayLayers;
    VkSampleCountFlagBits    samples;
    VkImageTiling            tiling;
    VkImageUsageFlags        usage; +
    VkSharingMode            sharingMode;
    uint32_t                 queueFamilyIndexCount;
    const uint32_t*          pQueueFamilyIndices;
    VkImageLayout            initialLayout;
  } VkImageCreateInfo;
  */

  VkImageCreateInfo image_info = {};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.pNext = nullptr;
  image_info.flags = 0;
  image_info.imageType = type;
  image_info.format = format;
  image_info.extent = extent;
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.usage = usage;
  return image_info;
}

inline VkImageViewCreateInfo
get_image_view_create_info(VkImage image, VkFormat format,
                           VkImageAspectFlags aspectFlags,
                           VkImageViewType type) {

  /*
  typedef struct VkImageViewCreateInfo {
    VkStructureType            sType; +
    const void*                pNext; +
    VkImageViewCreateFlags     flags;
    VkImage                    image; +
    VkImageViewType            viewType;
    VkFormat                   format; +
    VkComponentMapping         components;
    VkImageSubresourceRange    subresourceRange;
  } VkImageViewCreateInfo;
  */

  VkImageViewCreateInfo view_info = {};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.pNext = nullptr;
  view_info.flags = 0;
  view_info.image = image;
  view_info.viewType = type;
  view_info.format = format;

  view_info.subresourceRange.aspectMask = aspectFlags;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = 1;
  return view_info;
}

inline VmaAllocationCreateInfo
get_allocation_info(VmaMemoryUsage usage, VmaAllocationCreateFlags flags) {
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

inline VkWriteDescriptorSet
get_descriptor_write_info(size_t binding,                      //
                          VkDescriptorSet ds,                  //
                          VkDescriptorType type,               //
                          size_t descriptor_count,             //
                          VkDescriptorBufferInfo *buffer_info, //
                          VkDescriptorImageInfo *image_info) {
  VkWriteDescriptorSet write = {};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.pNext = nullptr;

  write.dstBinding = binding;
  write.dstSet = ds;
  write.descriptorCount = descriptor_count;
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

inline VkCommandPoolCreateInfo
get_command_pool_create_info(uint32_t queue_family_index,
                             VkCommandPoolCreateFlags flags) {
  VkCommandPoolCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  info.pNext = nullptr;

  info.queueFamilyIndex = queue_family_index;
  info.flags = flags;

  return info;
}

inline VkCommandBufferAllocateInfo
get_command_buffer_allocate_info(VkCommandPool pool, VkCommandBufferLevel level,
                                 uint32_t count) {
  VkCommandBufferAllocateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  info.pNext = nullptr;

  info.commandPool = pool;
  info.commandBufferCount = count;
  info.level = level;

  return info;
}

inline VkAttachmentDescription
get_attachment_description(VkFormat format,                      //
                           VkSampleCountFlagBits samples,        //
                           VkAttachmentLoadOp load_op,           //
                           VkAttachmentStoreOp store_op,         //
                           VkAttachmentLoadOp stencil_load_op,   //
                           VkAttachmentStoreOp stencil_store_op, //
                           VkImageLayout initial_layout,         //
                           VkImageLayout final_layout) {
  /*
  typedef struct VkAttachmentDescription {
    VkAttachmentDescriptionFlags    flags; Documentation has only one flag
    VkFormat                        format;
    VkSampleCountFlagBits           samples;
    VkAttachmentLoadOp              loadOp;
    VkAttachmentStoreOp             storeOp;
    VkAttachmentLoadOp              stencilLoadOp;
    VkAttachmentStoreOp             stencilStoreOp;
    VkImageLayout                   initialLayout;
    VkImageLayout                   finalLayout;
  } VkAttachmentDescription;
  */
  VkAttachmentDescription attachment = {};
  attachment.format = format;
  attachment.samples = samples;
  attachment.loadOp = load_op;
  attachment.storeOp = store_op;
  attachment.stencilLoadOp = stencil_load_op;
  attachment.stencilStoreOp = stencil_store_op;
  attachment.initialLayout = initial_layout;
  attachment.finalLayout = final_layout;
  return attachment;
}

inline VkSubpassDependency
get_subpass_dependency(uint32_t src_subpass,           //
                       uint32_t dst_subpass,           //
                       VkPipelineStageFlags src_stage, //
                       VkPipelineStageFlags dst_stage, //
                       VkAccessFlags src_access,       //
                       VkAccessFlags dst_access,       //
                       VkDependencyFlags flags) {
  /*
  typedef struct VkSubpassDependency {
    uint32_t                srcSubpass;
    uint32_t                dstSubpass;
    VkPipelineStageFlags    srcStageMask;
    VkPipelineStageFlags    dstStageMask;
    VkAccessFlags           srcAccessMask;
    VkAccessFlags           dstAccessMask;
    VkDependencyFlags       dependencyFlags;
  } VkSubpassDependency;
  */
  VkSubpassDependency dependency = {};
  dependency.srcSubpass = src_subpass;
  dependency.dstSubpass = dst_subpass;
  dependency.srcStageMask = src_stage;
  dependency.dstStageMask = dst_stage;
  dependency.srcAccessMask = src_access;
  dependency.dstAccessMask = dst_access;
  dependency.dependencyFlags = flags;
  return dependency;
}

inline VkSubpassDescription
get_subpass_description(VkSubpassDescriptionFlags flags,                      //
                        VkPipelineBindPoint pipelineBindPoint,                //
                        uint32_t inputAttachmentCount,                        //
                        const VkAttachmentReference *pInputAttachments,       //
                        uint32_t colorAttachmentCount,                        //
                        const VkAttachmentReference *pColorAttachments,       //
                        const VkAttachmentReference *pResolveAttachments,     //
                        const VkAttachmentReference *pDepthStencilAttachment, //
                        uint32_t preserveAttachmentCount,                     //
                        const uint32_t *pPreserveAttachments) {
  /*
    typedef struct VkSubpassDescription {
      VkSubpassDescriptionFlags       flags;
      VkPipelineBindPoint             pipelineBindPoint;
      uint32_t                        inputAttachmentCount;
      const VkAttachmentReference*    pInputAttachments;
      uint32_t                        colorAttachmentCount;
      const VkAttachmentReference*    pColorAttachments;
      const VkAttachmentReference*    pResolveAttachments;
      const VkAttachmentReference*    pDepthStencilAttachment;
      uint32_t                        preserveAttachmentCount;
      const uint32_t*                 pPreserveAttachments;
  } VkSubpassDescription;
  */
  VkSubpassDescription subpass = {};
  subpass.flags = flags;
  subpass.pipelineBindPoint = pipelineBindPoint;
  subpass.inputAttachmentCount = inputAttachmentCount;
  subpass.pInputAttachments = pInputAttachments;
  subpass.colorAttachmentCount = colorAttachmentCount;
  subpass.pColorAttachments = pColorAttachments;
  subpass.pResolveAttachments = pResolveAttachments;
  subpass.pDepthStencilAttachment = pDepthStencilAttachment;
  subpass.preserveAttachmentCount = preserveAttachmentCount;
  subpass.pPreserveAttachments = pPreserveAttachments;
  return subpass;
}

inline VkRenderPassCreateInfo
get_create_render_pass_info(VkRenderPassCreateFlags flags,               //
                            uint32_t attachmentCount,                    //
                            const VkAttachmentDescription *pAttachments, //
                            uint32_t subpassCount,                       //
                            const VkSubpassDescription *pSubpasses,      //
                            uint32_t dependencyCount,                    //
                            const VkSubpassDependency *pDependencies) {
  /*
  typedef struct VkRenderPassCreateInfo {
      VkStructureType                   sType;
      const void*                       pNext;
      VkRenderPassCreateFlags           flags;
      uint32_t                          attachmentCount;
      const VkAttachmentDescription*    pAttachments;
      uint32_t                          subpassCount;
      const VkSubpassDescription*       pSubpasses;
      uint32_t                          dependencyCount;
      const VkSubpassDependency*        pDependencies;
  } VkRenderPassCreateInfo;
  */
  VkRenderPassCreateInfo render_pass_info = {};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.pNext = nullptr;
  render_pass_info.flags = flags;
  render_pass_info.attachmentCount = attachmentCount;
  render_pass_info.pAttachments = pAttachments;
  render_pass_info.subpassCount = subpassCount;
  render_pass_info.pSubpasses = pSubpasses;
  render_pass_info.dependencyCount = dependencyCount;
  render_pass_info.pDependencies = pDependencies;
  return render_pass_info;
}
} // namespace vk_struct_init