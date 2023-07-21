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
                 init_descriptor_pool() && init_debug_renderable() &&
                 init_shadow_map() && init_materials();
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

  // Allocate buffers for the camera and model info and texture
  // This can be generalized to any number of buffers and descriptor sets
  // but will do for now
  _camera_buffer = BufferAllocation::create(
      sizeof(CameraInfo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, _allocator);
  _light_buffer = BufferAllocation::create(
      sizeof(LightSource) * MAX_LIGHTS, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, _allocator);

  _deallocator.push([&] {
    _camera_buffer.destroy();
    _light_buffer.destroy();
  });

  return true;
}

bool Renderer::init_materials() {
  { // Debug
    MaterialSpec mat_spec{};
    mat_spec.vert_shader = Path{ASSET_DIRECTORY} / "shaders/bin/debug_vert.spv";
    mat_spec.frag_shader = Path{ASSET_DIRECTORY} / "shaders/bin/debug_frag.spv";
    mat_spec.bindings = {
        {{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sizeof(CameraInfo), 1}}};
    mat_spec.topology_type = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    mat_spec.input_attributes =
        std::vector<VkFormat>{VK_FORMAT_R32G32B32_SFLOAT,  // Position
                              VK_FORMAT_R32G32B32_SFLOAT}; // Color
    debug_material = Material{mat_spec};                   // Color
    debug_material.build(_device, _descriptor_pool, _render_pass, _window_dims,
                         _deallocator);
  }

  { // Lights
    MaterialSpec mat_spec{};
    mat_spec.vert_shader = Path{ASSET_DIRECTORY} / "shaders/bin/light_vert.spv";
    mat_spec.frag_shader = Path{ASSET_DIRECTORY} / "shaders/bin/light_frag.spv";
    lights_material = Material{mat_spec}; // Color
    lights_material.build(_device, _descriptor_pool, _render_pass, _window_dims,
                          _deallocator);
  }

  { // Geometry
    MaterialSpec mat_spec{};
    geometry_material = Material{mat_spec}; // Color
    geometry_material.build(_device, _descriptor_pool, _render_pass,
                            _window_dims, _deallocator);
  }

  { // Shadow pass
    MaterialSpec mat_spec{};
    mat_spec.vert_shader =
        Path{ASSET_DIRECTORY} / "shaders/bin/shadow_map_vert.spv",
    mat_spec.frag_shader =
        Path{ASSET_DIRECTORY} / "shaders/bin/shadow_map_frag.spv",
    mat_spec.input_attributes =
        std::vector<VkFormat>{VK_FORMAT_R32G32B32_SFLOAT}; // Color

    // Have to set custom stride
    // in order to be able to use the same input buffer
    mat_spec.vertex_stride = FORMAT_SIZES[VK_FORMAT_R32G32B32_SFLOAT] +
                             FORMAT_SIZES[VK_FORMAT_R32G32B32_SFLOAT] +
                             FORMAT_SIZES[VK_FORMAT_R32G32_SFLOAT];

    shadow_material = Material{mat_spec};
    shadow_material.build(
        _device, _descriptor_pool, _shadow_pass.render_pass,
        VkExtent2D{SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION}, _deallocator);
  }
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
      VkExtent3D{_window_dims.width, _window_dims.height, 1}, VK_IMAGE_TYPE_2D);

  VkImageViewCreateInfo image_view_info =
      vk_struct_init::get_image_view_create_info(
          nullptr, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT,
          VK_IMAGE_VIEW_TYPE_2D); // Image arg will be substituted from
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

  VK_CHECK(
      vkAllocateCommandBuffers(_device, &cmd_buffer_info, &_command_buffer),
      "Failed to allocate command buffer");

  _deallocator.push([=] {
    vkDestroyCommandPool(_device, _submit_buffer.pool, nullptr);
    vkDestroyCommandPool(_device, _cmd_pool, nullptr);
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
  _deallocator.push(
      [=]() { vkDestroyRenderPass(_device, _render_pass, nullptr); });
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
    _deallocator.push([=] {
      vkDestroyFramebuffer(_device, _framebuffers[i], nullptr);
      vkDestroyImageView(_device, _swapchain_image_views[i], nullptr);
    });
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

  VkDescriptorPoolSize scene = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 200};
  VkDescriptorPoolSize material = {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                   100};

  VkDescriptorPoolSize pool_sizes[2] = {scene, material};

  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.pNext = nullptr;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 20;
  pool_info.poolSizeCount = sizeof(pool_sizes) / sizeof(VkDescriptorPoolSize);
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

  _deallocator.push([&] { debug_renderable.buffer.destroy(); });

  return true;
}

void Renderer::draw_debug_line(const Eigen::Vector3f &start,
                               const Eigen::Vector3f &end,
                               const Eigen::Vector3f &color) {
  if (!debug_mode) {
    return;
  }
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
  if (!debug_mode) {
    return;
  }
  for (int i = 0; i < 4; i++) {
    draw_debug_line(quad_verts.row(i), quad_verts.row((i + 1) % 4), color);
  }
}

void Renderer::draw_debug_box(const Eigen::Matrix<float, 8, 4> &box_verts,
                              const Eigen::Vector3f &color) {
  if (!debug_mode) {
    return;
  }
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
  if (!debug_mode) {
    return;
  }
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

bool Renderer::begin_one_time_submit() {
  VK_CHECK(vkResetCommandBuffer(_command_buffer, 0),
           "Failed to reset command buffer");
  VkCommandBufferBeginInfo cmd_buffer_info = {};
  cmd_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmd_buffer_info.pNext = nullptr;
  cmd_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkResetFences(_device, 1, &_submit_buffer.fence);
  VK_CHECK(vkBeginCommandBuffer(_command_buffer, &cmd_buffer_info),
           "Failed to begin command buffer");
  return true;
}

bool Renderer::end_one_time_submit() {
  VK_CHECK(vkEndCommandBuffer(_command_buffer), "Failed to end command buffer");

  VkSubmitInfo submit = {};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.pNext = nullptr;

  submit.waitSemaphoreCount = 0;
  submit.pWaitSemaphores = nullptr;
  submit.pWaitDstStageMask = nullptr;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &_command_buffer;
  submit.signalSemaphoreCount = 0;
  submit.pSignalSemaphores = nullptr;

  VK_CHECK(vkQueueSubmit(_graphics_queue, 1, &submit, _submit_buffer.fence),
           "Failed to submit to queue");
  vkWaitForFences(_device, 1, &_submit_buffer.fence, true, 9999999999);
  vkResetCommandPool(_device, _submit_buffer.pool, 0);
  return true;
}

bool Renderer::begin_command_buffer(VkCommandBuffer &command_buffer) {
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
  VK_CHECK(vkResetCommandBuffer(command_buffer, 0),
           "Failed to reset command buffer");

  VkCommandBufferBeginInfo cmd_buffer_info = {};
  cmd_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmd_buffer_info.pNext = nullptr;
  cmd_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  VK_CHECK(vkBeginCommandBuffer(command_buffer, &cmd_buffer_info),
           "Failed to begin command buffer");

  return true;
}

bool Renderer::begin_render_pass(VkRenderPass &render_pass,
                                 VkFramebuffer &framebuffer,
                                 VkCommandBuffer &command_buffer,
                                 const VkExtent2D &extent,
                                 float depth_clear_value,
                                 float color_clear_value) {
  std::vector<VkClearValue> clear_values;
  if (color_clear_value >= 0.0f) {
    VkClearValue clear_value{};
    clear_value.color = {
        {color_clear_value, color_clear_value, color_clear_value, 1.0f}};
    clear_values.push_back(clear_value);
  }

  if (depth_clear_value >= 0.0f) {
    VkClearValue depth_clear{};
    depth_clear.depthStencil = {depth_clear_value, 0};
    clear_values.push_back(depth_clear);
  }

  VkRenderPassBeginInfo rp_info = {};
  rp_info.clearValueCount = clear_values.size();
  rp_info.pClearValues = clear_values.data();

  rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rp_info.pNext = nullptr;

  rp_info.renderPass = render_pass;
  rp_info.renderArea.extent = extent;
  rp_info.framebuffer = framebuffer;

  vkCmdBeginRenderPass(command_buffer, &rp_info, VK_SUBPASS_CONTENTS_INLINE);
  return true;
}

void Renderer::end_render_pass(VkCommandBuffer &command_buffer) {
  vkCmdEndRenderPass(command_buffer);
}

bool Renderer::submit_command_buffer(VkCommandBuffer &command_buffer) {
  VK_CHECK(vkEndCommandBuffer(_command_buffer), "Failed to end command buffer");

  // Submit command to the render queue
  VkSubmitInfo submit = {};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.pNext = nullptr;
  submit.waitSemaphoreCount = 0;
  submit.pWaitSemaphores = nullptr;
  submit.pWaitDstStageMask = nullptr;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &_command_buffer;
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
  submit.pCommandBuffers = &_command_buffer;

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

// Allocate and transfer
void Renderer::transfer_texture_to_gpu(Texture::Ptr p_texture) {
  // Create a cpu side buffer for the texture
  BufferAllocation staging_buffer = BufferAllocation::create(
      p_texture->pixel_buffer.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, _allocator);

  staging_buffer.copy_from((void *)p_texture->pixel_buffer.pixels,
                           p_texture->pixel_buffer.size());

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
  barrier_info.image = p_texture->image_allocation.image;
  barrier_info.subresourceRange = range;

  barrier_info.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier_info.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

  barrier_info.srcAccessMask = 0;
  barrier_info.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  vkCmdPipelineBarrier(_command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier_info);
  // Copy the staging buffer to the image
  VkBufferImageCopy copyRegion = {};
  copyRegion.bufferOffset = 0;
  copyRegion.bufferRowLength = 0;
  copyRegion.bufferImageHeight = 0;
  copyRegion.imageExtent =
      VkExtent3D{p_texture->dims.width, p_texture->dims.height, 1};

  copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copyRegion.imageSubresource.mipLevel = 0;
  copyRegion.imageSubresource.baseArrayLayer = 0;
  copyRegion.imageSubresource.layerCount = 1;

  vkCmdCopyBufferToImage(_command_buffer, staging_buffer.buffer,
                         p_texture->image_allocation.image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

  VkImageMemoryBarrier barrier_info2 = {};
  barrier_info2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier_info2.image = p_texture->image_allocation.image;
  barrier_info2.subresourceRange = range;

  barrier_info2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier_info2.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier_info2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier_info2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier_info2);
  end_one_time_submit();

  staging_buffer.destroy(); // We don't need the staging buffer anymore
}

bool Renderer::init_shadow_map() {

  VkSamplerCreateInfo sampler_info = vk_struct_init::get_sampler_create_info(
      VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
  VK_CHECK(
      vkCreateSampler(_device, &sampler_info, nullptr, &_shadow_pass.sampler),
      "Failed to create sampler");

  _deallocator.push(
      [&] { vkDestroySampler(_device, _shadow_pass.sampler, nullptr); });

  // Allocate shadow mapping images
  VkImageCreateInfo image_info = vk_struct_init::get_image_create_info(
      VK_FORMAT_D32_SFLOAT,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      VkExtent3D{SHADOW_ATLAS_RESOLUTION, SHADOW_ATLAS_RESOLUTION, 1},
      VK_IMAGE_TYPE_2D);

  VkImageViewCreateInfo image_view_info =
      vk_struct_init::get_image_view_create_info(nullptr, VK_FORMAT_D32_SFLOAT,
                                                 VK_IMAGE_ASPECT_DEPTH_BIT,
                                                 VK_IMAGE_VIEW_TYPE_2D);

  VmaAllocationCreateInfo alloc_info = vk_struct_init::get_allocation_info(
      VMA_MEMORY_USAGE_GPU_ONLY, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  _shadow_pass.image_allocation = ImageAllocation::create(
      image_info, image_view_info, alloc_info, _device, _allocator);
  _deallocator.push([=] { _shadow_pass.image_allocation.destroy(); });

  // Create renderpass
  VkAttachmentDescription depth_attachment =
      vk_struct_init::get_attachment_description(
          _shadow_pass.image_allocation.format, //
          VK_SAMPLE_COUNT_1_BIT,                //
          VK_ATTACHMENT_LOAD_OP_CLEAR,          //
          VK_ATTACHMENT_STORE_OP_STORE,         //
          VK_ATTACHMENT_LOAD_OP_DONT_CARE,      //
          VK_ATTACHMENT_STORE_OP_DONT_CARE,     //
          VK_IMAGE_LAYOUT_UNDEFINED,            //
          _shadow_pass.layout                   //
      );

  VkAttachmentReference depth_attachment_ref = {
      0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  // This render pass depth tests depend on prev renderpass fragment shader
  VkSubpassDependency subpass_dep1 = vk_struct_init::get_subpass_dependency(
      VK_SUBPASS_EXTERNAL, //
      0,                   //
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, //
      VK_ACCESS_SHADER_READ_BIT,
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, //
      VK_DEPENDENCY_BY_REGION_BIT);

  // Next renderpass fragment shades depends on this renderpass write to depth
  // attachment
  VkSubpassDependency subpass_dep2 = vk_struct_init::get_subpass_dependency(
      0,
      VK_SUBPASS_EXTERNAL, //
      VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, //
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      VK_ACCESS_SHADER_READ_BIT, //
      VK_DEPENDENCY_BY_REGION_BIT);

  VkSubpassDependency subpass_dependencies[2] = {subpass_dep1, subpass_dep2};

  VkSubpassDescription subpass = vk_struct_init::get_subpass_description(
      0,                               //
      VK_PIPELINE_BIND_POINT_GRAPHICS, //
      0, nullptr,                      //
      0, nullptr,                      //
      nullptr,
      &depth_attachment_ref, //
      0, nullptr);

  // Define a renderpass
  VkRenderPassCreateInfo render_pass_info =
      vk_struct_init::get_create_render_pass_info(0,                    //
                                                  1, &depth_attachment, //
                                                  1, &subpass,          //
                                                  1, subpass_dependencies);
  VK_CHECK(vkCreateRenderPass(_device, &render_pass_info, nullptr,
                              &_shadow_pass.render_pass),
           "Failed to create renderpass");

  _deallocator.push([=]() {
    vkDestroyRenderPass(_device, _shadow_pass.render_pass, nullptr);
  });

  VkFramebufferCreateInfo fb_info = {};
  fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  fb_info.pNext = nullptr;
  fb_info.attachmentCount = 1;
  fb_info.renderPass = _shadow_pass.render_pass;
  fb_info.width = SHADOW_ATLAS_RESOLUTION;
  fb_info.height = SHADOW_ATLAS_RESOLUTION;
  fb_info.layers = 1;

  fb_info.pAttachments = &(_shadow_pass.image_allocation.view);
  VK_CHECK(vkCreateFramebuffer(_device, &fb_info, nullptr,
                               &_shadow_pass.framebuffer),
           "Failed to create shadow map framebuffer");
  _deallocator.push([=]() {
    vkDestroyFramebuffer(_device, _shadow_pass.framebuffer, nullptr);
  });

  return true;
}

void Renderer::check_renderables() {
  rend::ECS::EntityRegistry &registry = rend::ECS::get_entity_registry();

  for (rend::ECS::EntityRegistry::ArchetypeIterator rb_iterator =
           registry.archetype_iterator<Renderable, Transform>();
       rb_iterator.valid(); ++rb_iterator) {
    rend::ECS::EID eid = *rb_iterator;

    Renderable &renderable = registry.get_component<Renderable>(eid);

    if (renderable.p_mesh != nullptr &&
        !renderable.p_mesh->buffer_allocation.buffer_allocated) {
      renderable.p_mesh->generate_allocation_buffer(_allocator, _deallocator);
    }

    if (renderable.p_texture == nullptr) {
      renderable.p_texture = Texture::get_error_texture();
    }

    if (!renderable.p_texture->image_allocated()) {
      renderable.p_texture->allocate_image(_device, _allocator, _deallocator);
      transfer_texture_to_gpu(renderable.p_texture);
      if (texture_to_index.find(renderable.p_texture.get()) ==
          texture_to_index.end()) {
        texture_to_index.insert(
            {renderable.p_texture.get(), texture_to_index.size()});
      }
    }
  }

  static bool all_textures_allocated = false;
  if (!all_textures_allocated) {
    bind_textures();
    all_textures_allocated = true;
  }
}

bool Renderer::draw() {
  Eigen::Matrix4f projection = camera->projection;
  Eigen::Matrix4f view = camera->get_view_matrix();

  int debug_buffer_size = 0;

  void *datas[3] = {view.data(), projection.data(), camera->position.data()};
  size_t sizes[3] = {sizeof(float) * view.size(),
                     sizeof(float) * projection.size(), sizeof(float) * 3};

  _camera_buffer.copy_from(datas, sizes, 3);
  _light_buffer.copy_from(lights.data(), sizeof(LightSource) * lights.size());

  check_renderables();

  begin_command_buffer(_command_buffer);

  begin_render_pass(
      _shadow_pass.render_pass, _shadow_pass.framebuffer, _command_buffer,
      VkExtent2D{SHADOW_ATLAS_RESOLUTION, SHADOW_ATLAS_RESOLUTION}, 1.0f,
      -1.0f);

  shadow_material.bind_descriptor_buffer(1, 0, _camera_buffer);
  shadow_material.bind_descriptor_buffer(1, 1, _light_buffer);

  render_scene(_command_buffer, true);

  end_render_pass(_command_buffer);

  begin_render_pass(_render_pass, _framebuffers[_swapchain_img_idx],
                    _command_buffer, _window_dims, 1.0f, 0.0f);

  geometry_material.bind_descriptor_buffer(1, 0, _camera_buffer);
  geometry_material.bind_descriptor_buffer(1, 1, _light_buffer);
  geometry_material.ds_allocator.bind_image(1, 2, _shadow_pass.image_allocation,
                                            _shadow_pass.layout,
                                            _shadow_pass.sampler);
  lights_material.bind_descriptor_buffer(1, 0, _camera_buffer);
  lights_material.bind_descriptor_buffer(1, 1, _light_buffer);

  render_scene(_command_buffer, false);

  debug_material.bind_descriptor_buffer(0, 0, _camera_buffer);
  render_debug(_command_buffer);

  end_render_pass(_command_buffer);
  submit_command_buffer(_command_buffer);
  _frame_number++;
  return true;
}

void Renderer::bind_textures() {
  std::vector<VkDescriptorImageInfo> image_infos(MAX_TEXTURE_COUNT);
  for (auto iterator = texture_to_index.begin();
       iterator != texture_to_index.end(); iterator++) {
    Texture *texture = iterator->first;
    int index = iterator->second;
    image_infos[index] = VkDescriptorImageInfo{};
    image_infos[index].imageLayout = texture->layout;
    image_infos[index].imageView = texture->image_allocation.view;
    image_infos[index].sampler = texture->sampler;
  }

  int valid_texture_count = texture_to_index.size();
  auto dummy_texture = Texture::get_error_texture();
  for (int i = 0; i < MAX_TEXTURE_COUNT - valid_texture_count; i++) {
    image_infos[valid_texture_count + i] = VkDescriptorImageInfo{};
    image_infos[valid_texture_count + i].imageLayout = dummy_texture->layout;
    image_infos[valid_texture_count + i].imageView =
        dummy_texture->image_allocation.view;
    image_infos[valid_texture_count + i].sampler = dummy_texture->sampler;
  }

  geometry_material.ds_allocator.bind_image_infos(0, 0, image_infos);
  shadow_material.ds_allocator.bind_image_infos(0, 0, image_infos);
}

void Renderer::render_scene(VkCommandBuffer &command_buffer, bool shadow_pass) {
  rend::ECS::EntityRegistry &registry = rend::ECS::get_entity_registry();

  VkViewport viewport;
  VkRect2D scissor;
  if (shadow_pass) {
    viewport = {0, 0, SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION, 0, 1};
    scissor = {0, 0, SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION};
  } else {
    viewport = {0,
                0,
                static_cast<float>(_window_dims.width),
                static_cast<float>(_window_dims.height),
                0,
                1};
    scissor = {0, 0, _window_dims.width, _window_dims.height};
  }

  for (rend::ECS::EntityRegistry::ArchetypeIterator rb_iterator =
           registry.archetype_iterator<Renderable, Transform>();
       rb_iterator.valid(); ++rb_iterator) {
    rend::ECS::EID eid = *rb_iterator;

    Renderable &renderable = registry.get_component<Renderable>(eid);
    if (renderable.type == RenderableType::Light && shadow_pass) {
      continue;
    }
    Material &material = shadow_pass ? shadow_material
                         : renderable.type == RenderableType::Geometry
                             ? geometry_material
                             : lights_material;
    Mesh::Ptr mesh = renderable.p_mesh;

    Transform &transform = registry.get_component<Transform>(eid);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      material.pipeline);

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            material.pipeline_layout, 0,
                            material.ds_allocator.descriptor_sets.size(),
                            material.ds_allocator.descriptor_sets.data(), 0,
                            nullptr);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(command_buffer, 0, 1,
                           &mesh->buffer_allocation.buffer, &offset);

    PushConstants constants;
    Eigen::Matrix4f model = transform.get_model_matrix();
    Eigen::Matrix4f::Map(constants.model) = model;
    constants.texture_idx = texture_to_index[renderable.p_texture.get()];
    constants.light_index = 0;

    if (shadow_pass) {
      for (int light_idx = 0; light_idx < lights.size(); light_idx++) {
        if (!lights[light_idx].enabled()) {
          continue;
        }
        int light_x = light_idx % SHADOW_ATLAS_LIGHTS_PER_ROW;
        int light_y = light_idx / SHADOW_ATLAS_LIGHTS_PER_ROW;

        viewport = {static_cast<float>(light_x * SHADOW_MAP_RESOLUTION),
                    static_cast<float>(light_y * SHADOW_MAP_RESOLUTION),
                    SHADOW_MAP_RESOLUTION,
                    SHADOW_MAP_RESOLUTION,
                    0,
                    1};
        scissor = {light_x * SHADOW_MAP_RESOLUTION,
                   light_y * SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION,
                   SHADOW_MAP_RESOLUTION};

        vkCmdSetViewport(_command_buffer, 0, 1, &viewport);
        vkCmdSetScissor(_command_buffer, 0, 1, &scissor);

        constants.light_index = light_idx;
        vkCmdPushConstants(command_buffer, material.pipeline_layout,
                           material.spec.push_constants_description.stageFlags,
                           0, sizeof(PushConstants), &constants);
        vkCmdDraw(command_buffer, renderable.p_mesh->vertex_count(), 1, 0, 0);
      }
    } else {
      vkCmdSetViewport(_command_buffer, 0, 1, &viewport);
      vkCmdSetScissor(_command_buffer, 0, 1, &scissor);
      vkCmdPushConstants(command_buffer, material.pipeline_layout,
                         material.spec.push_constants_description.stageFlags, 0,
                         sizeof(PushConstants), &constants);
      vkCmdDraw(command_buffer, renderable.p_mesh->vertex_count(), 1, 0, 0);
    }
  }
}

void Renderer::render_debug(VkCommandBuffer &command_buffer) {
  if (debug_renderable.debug_verts_to_draw == 0) {
    return;
  }

  debug_renderable.buffer.copy_from(debug_renderable.debug_buffer,
                                    debug_renderable.debug_verts_to_draw * 6 *
                                        sizeof(float));
  VkDeviceSize offset = 0;
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    debug_material.pipeline);
  vkCmdBindVertexBuffers(command_buffer, 0, 1, &debug_renderable.buffer.buffer,
                         &offset);
  vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          debug_material.pipeline_layout, 0,
                          debug_material.ds_allocator.descriptor_sets.size(),
                          debug_material.ds_allocator.descriptor_sets.data(), 0,
                          nullptr);
  vkCmdDraw(command_buffer, debug_renderable.debug_verts_to_draw, 1, 0, 0);
  debug_renderable.debug_verts_to_draw = 0;
}
} // namespace rend