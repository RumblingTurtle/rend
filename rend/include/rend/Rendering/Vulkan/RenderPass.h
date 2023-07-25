#pragma once
#include <exception>
#include <rend/Rendering/Vulkan/Material.h>
#include <rend/Rendering/Vulkan/vk_helper_types.h>
#include <rend/Rendering/Vulkan/vk_struct_init.h>

struct AttachmentSpec {
  VkImageLayout layout;
  VkFormat format;

  VkExtent2D extent;

  VkFilter filter_mode;
  VkSamplerAddressMode address_mode;

  VkImageUsageFlags usage;
  VkImageUsageFlags aspect;

  bool active = false;
};

struct Attachment {
  ImageAllocation image_allocation;
  VkSampler sampler;

  VkDevice device_reference;
  AttachmentSpec spec;

  static Attachment create(AttachmentSpec spec, VkDevice device,
                           VmaAllocator allocator) {
    if (!spec.active) {
      throw std::runtime_error("Attachment spec not active");
    }

    Attachment attachment;
    attachment.spec = spec;
    attachment.device_reference = device;
    VkSamplerCreateInfo sampler_info = vk_struct_init::get_sampler_create_info(
        spec.filter_mode, spec.address_mode);
    if (vkCreateSampler(attachment.device_reference, &sampler_info, nullptr,
                        &attachment.sampler) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create sampler");
    }
    VkImageCreateInfo image_info = vk_struct_init::get_image_create_info(
        spec.format, spec.usage,
        VkExtent3D{spec.extent.width, spec.extent.height, 1}, VK_IMAGE_TYPE_2D);

    VkImageViewCreateInfo image_view_info =
        vk_struct_init::get_image_view_create_info(
            nullptr, spec.format, spec.aspect, VK_IMAGE_VIEW_TYPE_2D);

    VmaAllocationCreateInfo alloc_info = vk_struct_init::get_allocation_info(
        VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    attachment.image_allocation =
        ImageAllocation::create(image_info, image_view_info, alloc_info,
                                attachment.device_reference, allocator);
    return attachment;
  }

  void destroy() {
    image_allocation.destroy();
    vkDestroySampler(device_reference, sampler, nullptr);
  }
};

struct RenderPassSpec {
  struct SubpassDependency {
    VkPipelineStageFlagBits src_stage;
    VkPipelineStageFlagBits dst_stage;
    VkAccessFlagBits src_access;
    VkAccessFlagBits dst_access;
  };

  VkExtent2D extent;
  SubpassDependency prev_stage_dependency;
  SubpassDependency next_stage_dependency;
};

struct RenderPass {
  std::vector<Attachment> color_attachments;
  Attachment depth_attachment;

  VkRenderPass render_pass;
  VkFramebuffer framebuffer;

  Material material;

  VkDevice device_reference;

  RenderPassSpec spec;

  static RenderPass create(std::vector<AttachmentSpec> color_attachment_specs,
                           AttachmentSpec depth_attachment_spec,
                           MaterialSpec material_spec, RenderPassSpec pass_spec,
                           VkDevice device, VmaAllocator allocator) {
    RenderPass pass;
    pass.spec = pass_spec;
    std::vector<VkImageView> attachment_views(color_attachment_specs.size() +
                                              depth_attachment_spec.active);
    std::vector<VkAttachmentReference> attachment_references(
        color_attachment_specs.size());
    std::vector<VkAttachmentDescription> attachment_descriptions(
        color_attachment_specs.size() + depth_attachment_spec.active);

    pass.device_reference = device;
    pass.color_attachments.resize(color_attachment_specs.size());
    for (int i = 0; i < color_attachment_specs.size(); i++) {
      pass.color_attachments[i] =
          Attachment::create(color_attachment_specs[i], device, allocator);
      attachment_views[i] = pass.color_attachments[i].image_allocation.view;
      attachment_references[i] = {static_cast<uint32_t>(i),
                                  color_attachment_specs[i].layout};
      attachment_descriptions[i] = vk_struct_init::get_attachment_description(
          color_attachment_specs[i].format, //
          VK_SAMPLE_COUNT_1_BIT,            //
          VK_ATTACHMENT_LOAD_OP_CLEAR,      //
          VK_ATTACHMENT_STORE_OP_STORE,     //
          VK_ATTACHMENT_LOAD_OP_DONT_CARE,  //
          VK_ATTACHMENT_STORE_OP_DONT_CARE, //
          VK_IMAGE_LAYOUT_UNDEFINED,        //
          color_attachment_specs[i].layout  //
      );
    }

    VkAttachmentReference depth_attachment_reference = {
        static_cast<uint32_t>(color_attachment_specs.size()),
        depth_attachment_spec.layout};
    if (depth_attachment_spec.active) {
      pass.depth_attachment =
          Attachment::create(depth_attachment_spec, device, allocator);
      attachment_descriptions[color_attachment_specs.size()] =
          vk_struct_init::get_attachment_description(
              depth_attachment_spec.format,     //
              VK_SAMPLE_COUNT_1_BIT,            //
              VK_ATTACHMENT_LOAD_OP_CLEAR,      //
              VK_ATTACHMENT_STORE_OP_STORE,     //
              VK_ATTACHMENT_LOAD_OP_LOAD,       //
              VK_ATTACHMENT_STORE_OP_DONT_CARE, //
              VK_IMAGE_LAYOUT_UNDEFINED,        //
              depth_attachment_spec.layout      //
          );
      attachment_views[color_attachment_specs.size()] =
          pass.depth_attachment.image_allocation.view;
    }

    VkSubpassDependency prev_subpass_dep =
        vk_struct_init::get_subpass_dependency(
            VK_SUBPASS_EXTERNAL, //
            0,                   //
            pass_spec.prev_stage_dependency.src_stage,
            pass_spec.prev_stage_dependency.dst_stage, //
            pass_spec.prev_stage_dependency.src_access,
            pass_spec.prev_stage_dependency.dst_access, //
            VK_DEPENDENCY_BY_REGION_BIT);

    VkSubpassDependency next_subpass_dep =
        vk_struct_init::get_subpass_dependency(
            VK_SUBPASS_EXTERNAL, //
            0,                   //
            pass_spec.next_stage_dependency.src_stage,
            pass_spec.next_stage_dependency.dst_stage, //
            pass_spec.next_stage_dependency.src_access,
            pass_spec.next_stage_dependency.dst_access, //
            VK_DEPENDENCY_BY_REGION_BIT);

    VkSubpassDependency subpass_dependencies[2] = {prev_subpass_dep,
                                                   next_subpass_dep};

    VkSubpassDescription subpass_description =
        vk_struct_init::get_subpass_description(
            0,                                                          //
            VK_PIPELINE_BIND_POINT_GRAPHICS,                            //
            0, nullptr,                                                 //
            attachment_references.size(), attachment_references.data(), //
            nullptr,
            nullptr, //
            0, nullptr);

    subpass_description.pDepthStencilAttachment =
        depth_attachment_spec.active ? &depth_attachment_reference : nullptr;

    VkRenderPassCreateInfo render_pass_info =
        vk_struct_init::get_create_render_pass_info(
            0,                              //
            attachment_descriptions.size(), //
            attachment_descriptions.data(), //
            1, &subpass_description,        //
            2, subpass_dependencies);

    if (vkCreateRenderPass(pass.device_reference, &render_pass_info, nullptr,
                           &pass.render_pass) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create render pass");
    }

    VkFramebufferCreateInfo framebuffer_info = {};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.pNext = nullptr;
    framebuffer_info.renderPass = pass.render_pass;
    framebuffer_info.width = pass_spec.extent.width;
    framebuffer_info.height = pass_spec.extent.height;
    framebuffer_info.layers = 1;
    framebuffer_info.attachmentCount = attachment_views.size();
    framebuffer_info.pAttachments = attachment_views.data();

    if (vkCreateFramebuffer(pass.device_reference, &framebuffer_info, nullptr,
                            &pass.framebuffer) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create framebuffer");
    }
    pass.material = Material{material_spec};
    return pass;
  }

  void destroy() {
    vkDestroyFramebuffer(device_reference, framebuffer, nullptr);
    vkDestroyRenderPass(device_reference, render_pass, nullptr);
    for (int i = 0; i < color_attachments.size(); i++) {
      color_attachments[i].destroy();
    }
    if (depth_attachment.spec.active) {
      depth_attachment.destroy();
    }
  }
};
