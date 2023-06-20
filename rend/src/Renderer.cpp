#include "Renderer.h"

Renderer::~Renderer() {
  if (!initialized)
    return;

  // Wait for the command queue to complete if still running
  vkWaitForFences(_device, 1, &_command_complete_fence, VK_TRUE, 1000000000);
}

bool Renderer::init() {
  _window = SDL_CreateWindow("rend", SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED, _window_dims.width,
                             _window_dims.height, SDL_WINDOW_VULKAN);

  _deallocator.push([=]() { SDL_DestroyWindow(_window); });

  if (_window == nullptr) {
    std::cerr << "Failed to create _window: " << SDL_GetError() << std::endl;
    return false;
  }

  initialized = init_vulkan() && init_swapchain() && init_z_buffer() &&
                init_cmd_buffer() && init_renderpass() && init_framebuffers() &&
                init_sync_primitives() && init_pipelines();
  return initialized;
}

bool Renderer::init_vulkan() {
  vkb::InstanceBuilder builder;
  vkb::Result<vkb::Instance> inst_ret = builder.set_app_name("rend")
                                            .request_validation_layers()
                                            .use_default_debug_messenger()
                                            .build();
  if (!inst_ret) {
    std::cerr << "Failed to create Vulkan _instance. Error: "
              << inst_ret.error().message() << "\n";
    return false;
  }

  vkb::Instance vkb_inst = inst_ret.value();
  _instance = vkb_inst.instance;
  _debug_messenger = vkb_inst.debug_messenger;

  // Setting up devices and surfaces
  SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

  vkb::PhysicalDeviceSelector selector{vkb_inst};
  vkb::PhysicalDevice vkb_physical_device =
      selector.set_minimum_version(1, 1).set_surface(_surface).select().value();

  vkb::DeviceBuilder deviceBuilder{vkb_physical_device};

  vkb::Device vkb_device = deviceBuilder.build().value();

  _device = vkb_device.device;
  _physical_device = vkb_physical_device.physical_device;

  _graphics_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
  _queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

  // Setup vertex buffer allocator
  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice = _physical_device;
  allocatorInfo.device = _device;
  allocatorInfo.instance = _instance;
  vmaCreateAllocator(&allocatorInfo, &_allocator);

  _deallocator.push([=]() {
    vmaDestroyAllocator(_allocator);
    vkDestroyDevice(_device, nullptr);
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
    vkDestroyInstance(_instance, nullptr);
  });

  return true;
}

bool Renderer::init_swapchain() {
  vkb::SwapchainBuilder chainBuilder{_physical_device, _device, _surface};
  vkb::Swapchain vkb_swapchain =
      chainBuilder
          .use_default_format_selection()
          // use vsync present mode
          .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
          .set_desired_extent(_window_dims.width, _window_dims.height)
          .build()
          .value();

  _swapchain = vkb_swapchain.swapchain;
  _swapchain_images = vkb_swapchain.get_images().value();
  _swapchain_image_views = vkb_swapchain.get_image_views().value();
  _swapchain_image_format = vkb_swapchain.image_format;

  _deallocator.push(
      [=] { vkDestroySwapchainKHR(_device, _swapchain, nullptr); });
  return true;
}

bool Renderer::init_z_buffer() {
  VkImageCreateInfo image_info = vk_struct_init::get_image_create_info(
      _depth_image.format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      VkExtent3D{_window_dims.width, _window_dims.height, 1});

  VmaAllocationCreateInfo alloc_info = {};
  alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  alloc_info.requiredFlags =
      VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  vmaCreateImage(_allocator, &image_info, &alloc_info, &_depth_image.image,
                 &_depth_image.allocation, nullptr);

  VkImageViewCreateInfo image_view_info =
      vk_struct_init::get_image_view_create_info(
          _depth_image.image, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT);

  VK_CHECK(
      vkCreateImageView(_device, &image_view_info, nullptr, &_depth_image_view),
      "Failed to create depth image view");

  _deallocator.push([=] {
    vmaDestroyImage(_allocator, _depth_image.image, _depth_image.allocation);
    vkDestroyImageView(_device, _depth_image_view, nullptr);
  });
  return true;
}

bool Renderer::init_cmd_buffer() {
  VkCommandPoolCreateInfo cmd_pool_info = {};
  cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmd_pool_info.pNext = nullptr;

  cmd_pool_info.queueFamilyIndex = _queue_family;
  // allow  resetting of individual command buffers
  cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  VK_CHECK(vkCreateCommandPool(_device, &cmd_pool_info, nullptr, &_cmd_pool),
           "Failed to create command pool");

  VkCommandBufferAllocateInfo cmd_buffer_info = {};
  cmd_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmd_buffer_info.pNext = nullptr;
  cmd_buffer_info.commandPool = _cmd_pool;
  cmd_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmd_buffer_info.commandBufferCount = 1;

  VK_CHECK(vkAllocateCommandBuffers(_device, &cmd_buffer_info, &_cmd_buffer),
           "Failed to allocate command buffer");

  _deallocator.push([=] {
    vkDestroyCommandPool(_device, _cmd_pool, nullptr);

    vkDestroyRenderPass(_device, _render_pass, nullptr);
    // destroy swapchain resources
    for (int i = 0; i < _swapchain_image_views.size(); i++) {
      vkDestroyFramebuffer(_device, _framebuffers[i], nullptr);
      vkDestroyImageView(_device, _swapchain_image_views[i], nullptr);
    }
  });

  return true;
}

bool Renderer::init_renderpass() {
  VkAttachmentDescription color_attachment = {};
  color_attachment.format = _swapchain_image_format;
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear on load
  color_attachment.storeOp =
      VK_ATTACHMENT_STORE_OP_STORE; // Keep after renderpass
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  // must only be used for presenting a presentable image for display.
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentDescription depth_attachment = {};
  depth_attachment.format = _depth_image.format;
  depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depth_attachment.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription attachments[2] = {color_attachment, depth_attachment};

  // Attachments
  VkAttachmentReference color_attachment_ref = {};
  color_attachment_ref.attachment = 0; // attachment index
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depth_attachment_ref = {};
  depth_attachment_ref.attachment = 1; // attachment index
  depth_attachment_ref.layout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference attachment_refs[2] = {color_attachment_ref,
                                              depth_attachment_ref};

  VkSubpassDependency dependencies[2] = {};
  //  Color dependency
  dependencies[0].srcSubpass =
      VK_SUBPASS_EXTERNAL; // I'm not depending on a particular subpass but my
                           // previous self
  dependencies[0].dstSubpass = 0; // My subpass
  // I'm waiting on the color attachment stage in both source and destination
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  // Source is not specified so I'm waiting for the source to finish the color
  // attachment before I can write to the color attachment
  dependencies[0].srcAccessMask = 0; //
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  // Depth dependency
  dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].dstSubpass = 0;
  // I'm waiting for the depth tests from the previous subpass
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                 VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                 VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  // Can't start writing to depth attachment until previous subpass is done
  dependencies[1].srcAccessMask = 0;
  dependencies[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  // Define a subpass
  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment_ref;
  subpass.pDepthStencilAttachment = &depth_attachment_ref;

  // Define a renderpass
  VkRenderPassCreateInfo render_pass_info = {};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.pNext = nullptr;
  render_pass_info.attachmentCount = 2;
  render_pass_info.pAttachments = attachments;
  render_pass_info.subpassCount = 1;
  render_pass_info.pSubpasses = &subpass;
  render_pass_info.dependencyCount = 2;
  render_pass_info.pDependencies = dependencies;

  VK_CHECK(
      vkCreateRenderPass(_device, &render_pass_info, nullptr, &_render_pass),
      "Failed to create renderpass");

  return true;
}

bool Renderer::init_framebuffers() {
  // Framebuffers are connections between renderpass and images
  VkFramebufferCreateInfo fb_info = {};
  fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  fb_info.pNext = nullptr;
  fb_info.attachmentCount = 2;
  fb_info.renderPass = _render_pass;
  fb_info.width = _window_dims.width;
  fb_info.height = _window_dims.height;
  fb_info.layers = 1;

  // grab how many images we have in the swapchain
  const uint32_t swapchain_imagecount = _swapchain_images.size();
  _framebuffers.resize(swapchain_imagecount);

  // create framebuffers for each of the swapchain image views
  for (int i = 0; i < swapchain_imagecount; i++) {
    VkImageView attachments[2];
    attachments[0] = _swapchain_image_views[i];
    attachments[1] = _depth_image_view;

    fb_info.pAttachments = attachments;

    VK_CHECK(vkCreateFramebuffer(_device, &fb_info, nullptr, &_framebuffers[i]),
             "Failed to create framebuffer");
  }

  return true;
}

bool Renderer::init_sync_primitives() {
  VkSemaphoreCreateInfo semaphore_info = {};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  semaphore_info.pNext = nullptr;

  VkFenceCreateInfo fence_info = {};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.pNext = nullptr;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Creates waitable fence

  VK_CHECK(vkCreateSemaphore(_device, &semaphore_info, nullptr,
                             &_swapchain_semaphore),
           "Failed to create present semaphore");

  VK_CHECK(vkCreateSemaphore(_device, &semaphore_info, nullptr,
                             &_render_complete_semaphore),
           "Failed to create render semaphore");

  VK_CHECK(
      vkCreateFence(_device, &fence_info, nullptr, &_command_complete_fence),
      "Failed to create render fence");

  _deallocator.push([=] {
    vkDestroySemaphore(_device, _swapchain_semaphore, nullptr);
    vkDestroySemaphore(_device, _render_complete_semaphore, nullptr);
    vkDestroyFence(_device, _command_complete_fence, nullptr);
  });

  return true;
}

bool Renderer::init_pipelines() {
  Shader _triangle_vertex_shader{
      std::string{ASSET_DIRECTORY} + "/shaders/bin/triangle_vert.spv",
      std::string{ASSET_DIRECTORY} + "/shaders/bin/triangle_frag.spv"};
  VertexInfoDescription d{};
  VkPushConstantRange push_constant = {};
  push_constant.size = 0;
  add_pipeline(_triangle_vertex_shader, d, push_constant);

  return true;
}

bool Renderer::add_pipeline(Shader &shader,
                            VertexInfoDescription &vertex_info_description,
                            VkPushConstantRange &push_constant_range) {

  if (!shader.build_shader_modules(_device)) {
    return false;
  }

  _pipeline_layouts.resize(_pipeline_layouts.size() + 1);
  _pipelines.resize(_pipelines.size() + 1);
  _pipeline_builder.build_pipeline_layout(_device, _pipeline_layouts.back(),
                                          push_constant_range);
  _pipeline_builder.build_pipeline(_device, _render_pass, shader, _window_dims,
                                   _pipeline_layouts.back(),
                                   vertex_info_description, _pipelines.back());

  // Shader modules can be destroyed after pipeline creation
  shader.deinit(_device);

  int pipeline_idx = _pipelines.size() - 1;
  _deallocator.push([=] {
    vkDestroyPipeline(_device, _pipelines[pipeline_idx], nullptr);
    vkDestroyPipelineLayout(_device, _pipeline_layouts[pipeline_idx], nullptr);
  });

  return true;
}

bool Renderer::load_mesh(std::shared_ptr<Mesh> p_mesh,
                         std::shared_ptr<Object> p_object) {
  VkBufferCreateInfo buffer_info = {};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  // How much space do we need
  buffer_info.size = p_mesh->_vertices.size() * sizeof(Mesh::VertexInfo);
  buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

  // Writing buffer from CPU to GPU
  VmaAllocationCreateInfo vmaalloc_info = {};
  vmaalloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

  // Fill allocation buffer of the mesh object
  VK_CHECK(vmaCreateBuffer(_allocator, &buffer_info, &vmaalloc_info,
                           &p_mesh->_vertex_buffer._buffer,
                           &p_mesh->_vertex_buffer._allocation, nullptr),
           "Failed to create vertex buffer");

  // Destroy allocated buffer
  _deallocator.push([=]() {
    vmaDestroyBuffer(_allocator, p_mesh->_vertex_buffer._buffer,
                     p_mesh->_vertex_buffer._allocation);
  });

  void *data;
  vmaMapMemory(_allocator, p_mesh->_vertex_buffer._allocation, &data);
  memcpy(data, p_mesh->_vertices.data(),
         p_mesh->_vertices.size() * sizeof(Mesh::VertexInfo));
  vmaUnmapMemory(_allocator, p_mesh->_vertex_buffer._allocation);

  if (_pipelines.size() == 1) { // Limiting pipeline creation to 2
    add_pipeline(p_mesh->_shader, p_mesh->_vertex_info_description,
                 p_mesh->_push_constant_range);
  }

  _meshes.push_back(p_mesh);
  _objects.push_back(p_object);

  return true;
}

bool Renderer::begin_render_pass() {
  // Wait for the command queue to complete
  VK_CHECK(vkWaitForFences(_device, 1, &_command_complete_fence, VK_TRUE,
                           1000000000),
           "Render fence error");

  // Get next swapchain index
  VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000,
                                 _swapchain_semaphore, nullptr,
                                 &_swapchain_img_idx),
           "Failed to acquire swapchain image");

  // Fill command buffer
  VK_CHECK(vkResetCommandBuffer(_cmd_buffer, 0),
           "Failed to reset command buffer");

  VkCommandBufferBeginInfo cmd_buffer_info = {};
  cmd_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmd_buffer_info.pNext = nullptr;
  cmd_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  VK_CHECK(vkBeginCommandBuffer(_cmd_buffer, &cmd_buffer_info),
           "Failed to begin command buffer");

  VkRenderPassBeginInfo rp_info = {};
  rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rp_info.pNext = nullptr;

  rp_info.renderPass = _render_pass;
  rp_info.renderArea.offset.x = 0;
  rp_info.renderArea.offset.y = 0;
  rp_info.renderArea.extent = _window_dims;
  rp_info.framebuffer = _framebuffers[_swapchain_img_idx];

  VkClearValue clear_value;
  VkClearValue depth_clear;
  clear_value.color = {{0.5f, 0.5f, 0.5, 1.0f}};
  depth_clear.depthStencil.depth = 1.f;

  VkClearValue clear_values[2] = {clear_value, depth_clear};
  rp_info.clearValueCount = 2;
  rp_info.pClearValues = clear_values;

  vkCmdBeginRenderPass(_cmd_buffer, &rp_info, VK_SUBPASS_CONTENTS_INLINE);
  return true;
}

bool Renderer::end_render_pass() {
  vkCmdEndRenderPass(_cmd_buffer);

  VK_CHECK(vkEndCommandBuffer(_cmd_buffer), "Failed to end command buffer");

  // Submit command to the render queue
  VkSubmitInfo submit = {};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.pNext = nullptr;

  VkPipelineStageFlags wait_stage =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  // pointer to an array/value of pipeline stages at which each corresponding
  // semaphore wait will occur.
  submit.pWaitDstStageMask = &wait_stage;

  submit.waitSemaphoreCount = 1;
  submit.pWaitSemaphores = &_swapchain_semaphore; // Wait for swap chain image

  submit.signalSemaphoreCount = 1;
  submit.pSignalSemaphores =
      &_render_complete_semaphore; // Signal render complete

  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &_cmd_buffer;

  if (vkGetFenceStatus(_device, _command_complete_fence) == VK_SUCCESS) {
    vkResetFences(_device, 1, &_command_complete_fence);
  }

  VK_CHECK(vkQueueSubmit(_graphics_queue, 1, &submit, _command_complete_fence),
           "Failed to submit to queue");

  // Present image from the swapchain when pipeline is finished
  VkPresentInfoKHR present_info = {};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.pNext = nullptr;

  present_info.pSwapchains = &_swapchain;
  present_info.swapchainCount = 1;

  present_info.pWaitSemaphores = &_render_complete_semaphore;
  present_info.waitSemaphoreCount = 1;

  present_info.pImageIndices = &_swapchain_img_idx;

  VK_CHECK(vkQueuePresentKHR(_graphics_queue, &present_info),
           "Failed to present image");
  return true;
}

bool Renderer::draw() {
  begin_render_pass();

  // Binding first pipeline which is a single triangle
  //{
  //  vkCmdBindPipeline(_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
  //                    _pipelines[0]);
  //  vkCmdDraw(_cmd_buffer, 3, 1, 0, 0);
  //}

  Eigen::Matrix4f projection = _camera->get_projection_matrix();
  Eigen::Matrix4f view = _camera->get_view_matrix();

  Mesh::ShaderConstants constants;
  memcpy(constants.view, view.data(), sizeof(float) * 16);
  memcpy(constants.projection, projection.data(), sizeof(float) * 16);

  vkCmdBindPipeline(_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    _pipelines[1]);
  for (int m_idx = 0; m_idx < _meshes.size(); m_idx++) {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(_cmd_buffer, 0, 1,
                           &_meshes[m_idx]->_vertex_buffer._buffer, &offset);

    Eigen::Matrix4f model = _objects[m_idx]->get_model_matrix();

    memcpy(constants.model, model.data(), sizeof(float) * 16);

    vkCmdPushConstants(_cmd_buffer, _pipeline_layouts[1],
                       VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(Mesh::ShaderConstants), &constants);

    vkCmdDraw(_cmd_buffer, _meshes[m_idx]->_vertices.size(), 1, 0, 0);
  }

  end_render_pass();

  frame_number++;
  return true;
}
