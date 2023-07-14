#include <rend/Rendering/Vulkan/Renderer.h>

namespace rend {
void Renderer::cleanup() {
  if (!_initialized)
    return;

  // Wait for the command queue to complete if still running
  vkWaitForFences(_device, 1, &_command_complete_fence, VK_TRUE, 1000000000);
  _deallocator.cleanup();
}

bool Renderer::init() {
  _window = SDL_CreateWindow("rend", SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED, _window_dims.width,
                             _window_dims.height, SDL_WINDOW_VULKAN);
  // SDL_SetRelativeMouseMode(SDL_TRUE);

  _deallocator.push([=]() { SDL_DestroyWindow(_window); });

  if (_window == nullptr) {
    std::cerr << "Failed to create _window: " << SDL_GetError() << std::endl;
    return false;
  }

  _initialized = init_vulkan() && init_swapchain() && init_z_buffer() &&
                 init_cmd_buffer() && init_renderpass() &&
                 init_framebuffers() && init_sync_primitives() &&
                 init_descriptor_pool();
  return _initialized;
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

  min_ubo_alignment = vkb_device.physical_device.properties.limits
                          .minUniformBufferOffsetAlignment;

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
      chainBuilder.use_default_format_selection()
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
      VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      VkExtent3D{_window_dims.width, _window_dims.height, 1});

  VkImageViewCreateInfo image_view_info =
      vk_struct_init::get_image_view_create_info(
          nullptr, VK_FORMAT_D32_SFLOAT,
          VK_IMAGE_ASPECT_DEPTH_BIT); // Image arg will be substituted from
                                      // image_infos image field

  VmaAllocationCreateInfo alloc_info = vk_struct_init::get_allocation_info(
      VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  _depth_image = ImageAllocation::create(image_info, image_view_info,
                                         alloc_info, _device, _allocator);

  _deallocator.push([=] { _depth_image.destroy(); });
  return true;
}

bool Renderer::init_cmd_buffer() {
  VkCommandPoolCreateInfo cmd_pool_info =
      vk_struct_init::get_command_pool_create_info(
          _queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  VK_CHECK(vkCreateCommandPool(_device, &cmd_pool_info, nullptr, &_cmd_pool),
           "Failed to create command pool");
  VK_CHECK(vkCreateCommandPool(_device, &cmd_pool_info, nullptr,
                               &_submit_buffer.pool),
           "Failed to create command pool");

  VkCommandBufferAllocateInfo cmd_buffer_info =
      vk_struct_init::get_command_buffer_allocate_info(
          _cmd_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
  VkCommandBufferAllocateInfo submit_buffer_info =
      vk_struct_init::get_command_buffer_allocate_info(
          _submit_buffer.pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

  VK_CHECK(vkAllocateCommandBuffers(_device, &cmd_buffer_info, &_cmd_buffer),
           "Failed to allocate command buffer");
  VK_CHECK(vkAllocateCommandBuffers(_device, &submit_buffer_info,
                                    &_submit_buffer.buffer),
           "Failed to allocate submit buffer");

  _deallocator.push([=] {
    vkDestroyCommandPool(_device, _submit_buffer.pool, nullptr);
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
  VkAttachmentDescription color_attachment =
      vk_struct_init::get_attachment_description(
          _swapchain_image_format,          //
          VK_SAMPLE_COUNT_1_BIT,            //
          VK_ATTACHMENT_LOAD_OP_CLEAR,      //
          VK_ATTACHMENT_STORE_OP_STORE,     //
          VK_ATTACHMENT_LOAD_OP_DONT_CARE,  //
          VK_ATTACHMENT_STORE_OP_DONT_CARE, //
          VK_IMAGE_LAYOUT_UNDEFINED,        //
          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR   //
      );

  VkAttachmentDescription depth_attachment =
      vk_struct_init::get_attachment_description(
          _depth_image.format,                             //
          VK_SAMPLE_COUNT_1_BIT,                           //
          VK_ATTACHMENT_LOAD_OP_CLEAR,                     //
          VK_ATTACHMENT_STORE_OP_STORE,                    //
          VK_ATTACHMENT_LOAD_OP_LOAD,                      //
          VK_ATTACHMENT_STORE_OP_DONT_CARE,                //
          VK_IMAGE_LAYOUT_UNDEFINED,                       //
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL //
      );

  VkAttachmentDescription attachments[2] = {color_attachment, depth_attachment};

  // Attachments
  VkAttachmentReference color_attachment_ref = {
      0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  VkAttachmentReference depth_attachment_ref = {
      1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  VkSubpassDependency color_dependency = vk_struct_init::get_subpass_dependency(
      // - Wait for the previous subpass to finish
      VK_SUBPASS_EXTERNAL, //
      0,
      // - I'm waiting on the
      // color attachment stage
      // in both source and
      // destination
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,

      // - Source is not specified
      // so I'm waiting for the
      // source to finish the
      // color attachment before
      // I can write to the color
      // attachment
      0,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, //
      0);

  VkSubpassDependency depth_dependency = vk_struct_init::get_subpass_dependency(
      VK_SUBPASS_EXTERNAL, //
      0,
      // I'm waiting for the depth tests from the previous subpass
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
          VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
          VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
      // Can't start writing to depth attachment until previous subpass is done
      0,
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, //
      0);

  VkSubpassDependency subpass_dependencies[2] = {color_dependency,
                                                 depth_dependency};

  VkSubpassDescription subpass = vk_struct_init::get_subpass_description(
      0,                               //
      VK_PIPELINE_BIND_POINT_GRAPHICS, //
      0, nullptr,                      //
      1, &color_attachment_ref,        //
      nullptr,
      &depth_attachment_ref, //
      0, nullptr);

  // Define a renderpass
  VkRenderPassCreateInfo render_pass_info =
      vk_struct_init::get_create_render_pass_info(0,              //
                                                  2, attachments, //
                                                  1, &subpass,    //
                                                  2, subpass_dependencies);

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
    VkImageView attachments[2] = {_swapchain_image_views[i], _depth_image.view};
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
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  VK_CHECK(vkCreateSemaphore(_device, &semaphore_info, nullptr,
                             &_swapchain_semaphore),
           "Failed to create present semaphore");

  VK_CHECK(vkCreateSemaphore(_device, &semaphore_info, nullptr,
                             &_render_complete_semaphore),
           "Failed to create render semaphore");

  VK_CHECK(
      vkCreateFence(_device, &fence_info, nullptr, &_command_complete_fence),
      "Failed to create render fence");

  VK_CHECK(vkCreateFence(_device, &fence_info, nullptr, &_submit_buffer.fence),
           "Failed to create submit fence");

  _deallocator.push([=] {
    vkDestroySemaphore(_device, _swapchain_semaphore, nullptr);
    vkDestroySemaphore(_device, _render_complete_semaphore, nullptr);
    vkDestroyFence(_device, _command_complete_fence, nullptr);
    vkDestroyFence(_device, _submit_buffer.fence, nullptr);
  });

  return true;
}

bool Renderer::init_descriptor_pool() {
  VkDescriptorPoolCreateInfo pool_info = {};

  VkDescriptorPoolSize scene = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 20};
  VkDescriptorPoolSize material = {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                   20};

  VkDescriptorPoolSize model = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 20};

  VkDescriptorPoolSize pool_sizes[3] = {scene, material, model};

  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.pNext = nullptr;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 10;
  pool_info.poolSizeCount = 3;
  pool_info.pPoolSizes = pool_sizes;
  if (vkCreateDescriptorPool(_device, &pool_info, nullptr, &_descriptor_pool) !=
      VK_SUCCESS) {
    return false;
  }

  _deallocator.push(
      [=] { vkDestroyDescriptorPool(_device, _descriptor_pool, nullptr); });
  return true;
}

bool Renderer::init_debug_renderable() {
  debug_renderable.buffer = BufferAllocation::create(
      3 * sizeof(float) * MAX_DEBUG_VERTICES, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, _allocator);
  debug_renderable.material =
      Material{Path{ASSET_DIRECTORY} / "shaders/bin/debug_vert.spv",
               Path{ASSET_DIRECTORY} / "shaders/bin/debug_frag.spv",
               {},
               std::vector<VkFormat>{VK_FORMAT_R32G32B32_SFLOAT,   // Position
                                     VK_FORMAT_R32G32B32_SFLOAT}}; // Color

  debug_renderable.material.topology_type = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

  init_material(debug_renderable.material);
  _deallocator.push([&] { debug_renderable.buffer.destroy(); });

  return true;
}

void Renderer::draw_debug_line(const Eigen::Vector3f &start,
                               const Eigen::Vector3f &end,
                               const Eigen::Vector3f &color) {
  if (MAX_DEBUG_VERTICES <= debug_renderable.debug_verts_to_draw * 2) {
    throw std::runtime_error("Debug buffer overflow");
  }
  // TODO: Replace with vert incices
  int start_offset = debug_renderable.debug_verts_to_draw * 6;
  memcpy(debug_renderable.debug_buffer + start_offset, start.data(),
         sizeof(float) * 3);
  memcpy(debug_renderable.debug_buffer + start_offset + 3, color.data(),
         sizeof(float) * 3);
  memcpy(debug_renderable.debug_buffer + start_offset + 6, end.data(),
         sizeof(float) * 3);
  memcpy(debug_renderable.debug_buffer + start_offset + 9, color.data(),
         sizeof(float) * 3);
  debug_renderable.debug_verts_to_draw += 2;
}

void Renderer::draw_debug_quad(const Eigen::Matrix<float, 4, 3> &quad_verts,
                               const Eigen::Vector3f &color) {
  for (int i = 0; i < 4; i++) {
    draw_debug_line(quad_verts.row(i), quad_verts.row((i + 1) % 4), color);
  }
}

void Renderer::draw_debug_box(const Eigen::Matrix<float, 8, 4> &box_verts,
                              const Eigen::Vector3f &color) {
  /*
  Vertex order is :
  0-3 top clockwise
  4-7 bottom clockwise
  */
  for (int edge = 0; edge < 4; edge++) {
    int b1 = edge % 4;
    int b2 = (edge + 1) % 4;

    int t1 = 4 + edge % 4;
    int t2 = 4 + (edge + 1) % 4;
    /*

              Top strip
              t1 ---- t2/|
    Side strip|        | |
              |        | |
              b1 ---- b2/
              Bottom strip
    */
    draw_debug_line(box_verts.row(b1).head<3>(), box_verts.row(b2).head<3>(),
                    color);
    draw_debug_line(box_verts.row(t1).head<3>(), box_verts.row(t2).head<3>(),
                    color);
    draw_debug_line(box_verts.row(t1).head<3>(), box_verts.row(b1).head<3>(),
                    color);
  }
}

void Renderer::draw_debug_sphere(const Eigen::Vector3f &position, float radius,
                                 int resolution, const Eigen::Vector3f &color) {

  Eigen::MatrixXf sphere_verts(resolution, 3);
  for (int i = 0; i < resolution; i++) {
    float theta = i * 2 * M_PI / resolution;
    sphere_verts.row(i) =
        Eigen::Vector3f{radius * cos(theta), radius * sin(theta),
                        0}; // No rotation around Y axis
  }

  for (int i = 0; i < resolution; i++) {
    Eigen::Matrix3f rotation =
        Eigen::AngleAxisf(i * 2 * M_PI / resolution, Eigen::Vector3f::UnitY())
            .toRotationMatrix();

    Eigen::MatrixXf rotated_verts = rotation * sphere_verts.transpose();
    for (int i = 0; i < resolution; i++) {
      draw_debug_line(position + rotated_verts.col(i),
                      position + rotated_verts.col((i + 1) % resolution),
                      color);
    }
  }

  // Horizontal ring
  Eigen::Matrix3f rotation =
      Eigen::AngleAxisf(M_PI_2, Eigen::Vector3f::UnitX()).toRotationMatrix();

  Eigen::MatrixXf rotated_verts = rotation * sphere_verts.transpose();
  for (int i = 0; i < resolution; i++) {
    draw_debug_line(position + rotated_verts.col(i),
                    position + rotated_verts.col((i + 1) % resolution), color);
  }
}

bool Renderer::load_renderable(rend::ECS::EID r_id) {

  rend::ECS::EntityRegistry &registry = rend::ECS::get_entity_registry();
  if (!registry.is_component_enabled<Renderable>(r_id)) {
    std::cerr << "Entity " << r_id << " does not have a renderable component"
              << std::endl;
    return false;
  }

  Renderable &renderable = registry.get_component<Renderable>(r_id);
  Material *p_material = renderable.p_material.get();

  if (p_material == nullptr) {
    std::cerr << "Renderer::load_renderable material is null" << std::endl;
    return false;
  }
  if (_renderables.find(p_material) == _renderables.end()) {
    _renderables[p_material] =
        std::vector<std::pair<Renderable, rend::ECS::EID>>();
  }
  renderable.p_mesh->generate_allocation_buffer(_allocator, _deallocator);
  _renderables[p_material].push_back(std::make_pair(renderable, r_id));
  return true;
}

bool Renderer::begin_one_time_submit() {
  VK_CHECK(vkResetCommandBuffer(_submit_buffer.buffer, 0),
           "Failed to reset command buffer");
  VkCommandBufferBeginInfo cmd_buffer_info = {};
  cmd_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmd_buffer_info.pNext = nullptr;
  cmd_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkResetFences(_device, 1, &_submit_buffer.fence);
  VK_CHECK(vkBeginCommandBuffer(_submit_buffer.buffer, &cmd_buffer_info),
           "Failed to begin command buffer");
  return true;
}

bool Renderer::end_one_time_submit() {
  VK_CHECK(vkEndCommandBuffer(_submit_buffer.buffer),
           "Failed to end command buffer");

  VkSubmitInfo submit = {};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.pNext = nullptr;

  submit.waitSemaphoreCount = 0;
  submit.pWaitSemaphores = nullptr;
  submit.pWaitDstStageMask = nullptr;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &_submit_buffer.buffer;
  submit.signalSemaphoreCount = 0;
  submit.pSignalSemaphores = nullptr;

  VK_CHECK(vkQueueSubmit(_graphics_queue, 1, &submit, _submit_buffer.fence),
           "Failed to submit to queue");
  vkWaitForFences(_device, 1, &_submit_buffer.fence, true, 9999999999);
  vkResetCommandPool(_device, _submit_buffer.pool, 0);
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
  clear_value.color = {{0.2f, 0.2f, 0.2, 1.0f}};
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
  submit.waitSemaphoreCount = 0;
  submit.pWaitSemaphores = nullptr;
  submit.pWaitDstStageMask = nullptr;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &_cmd_buffer;
  submit.signalSemaphoreCount = 0;
  submit.pSignalSemaphores = nullptr;

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

  vkResetFences(_device, 1,
                &_command_complete_fence); // Can be safely reset because we
                                           // already waited on it
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

// Check if the material has been built, if not build it and send it's texture
// to GPU
bool Renderer::check_materials() {
  if (debug_renderable.buffer.buffer == VK_NULL_HANDLE) {
    init_debug_renderable();
  }
  for (auto &pair : _renderables) {
    if (!pair.first->pipeline_built()) {
      init_material(*pair.first);
    }
  }
  return true;
}

void Renderer::init_material(Material &material) {
  material.init(_device, _allocator, _descriptor_pool, _deallocator);
  material.build_pipeline(_device, _render_pass, _window_dims, _deallocator);
  if (material.texture.valid) {
    transfer_texture_to_gpu(material.texture);
  }
}

void Renderer::transfer_texture_to_gpu(Texture &texture) {
  // Create a cpu side buffer for the texture
  // Create a cpu side buffer for the texture
  BufferAllocation staging_buffer = BufferAllocation::create(
      texture.get_pixels_size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, _allocator);

  staging_buffer.copy_from((void *)texture.get_pixels(),
                           texture.get_pixels_size());

  // Change the layout of the texture to be linear as the staging buffer
  VkImageSubresourceRange range;
  range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  range.baseMipLevel = 0;
  range.levelCount = 1;
  range.baseArrayLayer = 0;
  range.layerCount = 1;

  begin_one_time_submit();
  VkImageMemoryBarrier barrier_info = {};
  barrier_info.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier_info.image = texture.image_allocation.image;
  barrier_info.subresourceRange = range;

  barrier_info.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier_info.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

  barrier_info.srcAccessMask = 0;
  barrier_info.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  vkCmdPipelineBarrier(_submit_buffer.buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier_info);
  // Copy the staging buffer to the image
  VkBufferImageCopy copyRegion = {};
  copyRegion.bufferOffset = 0;
  copyRegion.bufferRowLength = 0;
  copyRegion.bufferImageHeight = 0;

  copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copyRegion.imageSubresource.mipLevel = 0;
  copyRegion.imageSubresource.baseArrayLayer = 0;
  copyRegion.imageSubresource.layerCount = 1;
  copyRegion.imageExtent = VkExtent3D{static_cast<uint32_t>(texture.width),
                                      static_cast<uint32_t>(texture.height), 1};

  vkCmdCopyBufferToImage(_submit_buffer.buffer, staging_buffer.buffer,
                         texture.image_allocation.image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

  VkImageMemoryBarrier barrier_info2 = {};
  barrier_info2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier_info2.image = texture.image_allocation.image;
  barrier_info2.subresourceRange = range;

  barrier_info2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier_info2.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier_info2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier_info2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(_submit_buffer.buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier_info2);
  end_one_time_submit();

  staging_buffer.destroy(); // We don't need the staging buffer anymore
}

bool Renderer::draw() {
  rend::ECS::EntityRegistry &entity_registry = rend::ECS::get_entity_registry();

  check_materials();

  begin_render_pass();

  Eigen::Matrix4f projection = camera->get_projection_matrix();
  Eigen::Matrix4f view = camera->get_view_matrix();

  int debug_buffer_size = 0;

  Material *prev_material = nullptr;
  for (auto &pair : _renderables) {
    Material *material = pair.first;
    if (prev_material != material) {
      vkCmdBindPipeline(_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        material->pipeline);
      vkCmdBindDescriptorSets(_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              material->pipeline_layout, 0,
                              material->ds_allocator.descriptor_sets.size(),
                              material->ds_allocator.descriptor_sets.data(), 0,
                              nullptr);

      void *datas[3] = {view.data(), projection.data(),
                        camera->position.data()};
      size_t sizes[3] = {sizeof(float) * view.size(),
                         sizeof(float) * projection.size(), sizeof(float) * 3};

      material->_camera_buffer.copy_from(datas, sizes, 3);
      material->update_lights(lights);
    }

    for (std::pair<Renderable, rend::ECS::EID> &renderable_data : pair.second) {
      VkDeviceSize offset = 0;
      PushConstants constants;
      Renderable &renderable = renderable_data.first;
      Transform &object =
          entity_registry.get_component<Transform>(renderable_data.second);

      Eigen::Matrix4f model = object.get_model_matrix();
      memcpy(constants.model, model.data(), sizeof(float) * model.size());

      { // Mesh
        vkCmdBindVertexBuffers(_cmd_buffer, 0, 1,
                               &renderable.p_mesh->buffer_allocation.buffer,
                               &offset);

        vkCmdPushConstants(_cmd_buffer, material->pipeline_layout,
                           VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float),
                           &constants);
        vkCmdDraw(_cmd_buffer, renderable.p_mesh->vertex_count(), 1, 0, 0);
      }
    }
  }

  if (debug_renderable.debug_verts_to_draw > 0) {
    debug_renderable.buffer.copy_from(debug_renderable.debug_buffer,
                                      debug_renderable.debug_verts_to_draw * 6 *
                                          sizeof(float));
    VkDeviceSize offset = 0;
    vkCmdBindPipeline(_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      debug_renderable.material.pipeline);
    vkCmdBindVertexBuffers(_cmd_buffer, 0, 1, &debug_renderable.buffer.buffer,
                           &offset);

    vkCmdDraw(_cmd_buffer, debug_renderable.debug_verts_to_draw, 1, 0, 0);
    debug_renderable.debug_verts_to_draw = 0;
  }

  end_render_pass();

  _frame_number++;
  return true;
}
} // namespace rend