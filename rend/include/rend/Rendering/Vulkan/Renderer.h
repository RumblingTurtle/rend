#pragma once

#include <exception>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <rend/Rendering/Vulkan/Mesh.h>
#include <rend/Rendering/Vulkan/RenderPass.h>
#include <rend/Rendering/Vulkan/Renderable.h>
#include <rend/Rendering/Vulkan/vk_helper_types.h>
#include <rend/Rendering/Vulkan/vk_struct_init.h>
#include <rend/Transform.h>
#include <rend/macros.h>

#include <rend/EntityRegistry.h>

namespace rend {
class Renderer {
  static constexpr int MAX_DEBUG_VERTICES = 100000;
  static constexpr int SHADOW_ATLAS_LIGHTS_PER_ROW = 8;
  static constexpr int SHADOW_MAP_RESOLUTION = 1024;
  static constexpr int SHADOW_ATLAS_RESOLUTION =
      SHADOW_ATLAS_LIGHTS_PER_ROW * SHADOW_MAP_RESOLUTION;

  VkExtent2D _window_dims{1280, 1024};
  SDL_Window *_window;
  VkInstance _instance;
  VkPhysicalDevice _physical_device;
  VkDevice _device;
  VkSurfaceKHR _surface;
  VkQueue _graphics_queue;
  uint32_t _queue_family;
  VkDeviceSize min_ubo_alignment;

  VkAllocationCallbacks *_allocation_callbacks;

  int _frame_number = 0;
  bool _initialized = false;

  VkDebugUtilsMessengerEXT _debug_messenger;

  // Swapchain
  VkSwapchainKHR _swapchain;
  uint32_t _swapchain_img_idx; // Current swapchain image index

  // CMD buffer
  VkCommandPool _command_pool;
  VkCommandBuffer _command_buffer;

  VkFence _submit_fence;

  VkDescriptorPool _descriptor_pool;

  // Sync primitives
  VkSemaphore _swapchain_semaphore; // Signaled when the next swapchain image
                                    // index is aquired
  VkSemaphore _render_complete_semaphore; // Signaled when rendering is done
  VkFence _command_complete_fence; // Fence for waiting on the command queue

  // VMA allocator
  VmaAllocator _allocator;
  Deallocator _deallocation_queue;

  BufferAllocation _camera_buffer;
  BufferAllocation _light_buffer;

public:
  RenderPass deferred_pass;
  RenderPass shadow_pass;
  RenderPass shading_pass;
  RenderPass screenspace_effects_pass;
  RenderPass screenspace_smoothing_pass;

  struct {
    std::vector<VkImage> images;
    std::vector<VkImageView> image_views;

    ImageAllocation depth_buffer;

    VkRenderPass render_pass;
    std::vector<VkFramebuffer> framebuffers;

    VkFormat color_format;
    VkColorSpaceKHR color_space;
    VkFormat depth_format = VK_FORMAT_D32_SFLOAT;

    Material material;

    BufferAllocation buffer_allocation;
    // Position/Color
    float debug_buffer[MAX_DEBUG_VERTICES * sizeof(float) * 6];
    int debug_verts_to_draw = 0;
    Material debug_material;

  } composite_pass;

  std::unique_ptr<Camera> camera;
  std::vector<LightSource> lights;

  std::unordered_map<Texture *, int> texture_to_index;

  bool debug_mode = false;
  bool show_gui = false;

  Renderer();

  void init();

  // Initializes physical and logical devices + VBA
  void init_vulkan();

  // Allocates swapchain, it's image buffers and image views into the buffers
  void init_swapchain();

  // Allocates the depth image and it't view
  void init_z_buffer();

  // Initializes the command pool to be submitted to the graphics queue
  void init_cmd_buffer();

  // Defines the renderpass with all the subpasses and their attachments
  void init_renderpass();

  // Render passes work on framebuffers not directly on image views
  void init_framebuffers();

  // Initializes semaphores and fences for syncronization with the gpu
  void init_sync_primitives();

  void init_descriptor_pool();

  void init_debug_renderable();

  // Starts submission command buffer recording
  void begin_one_time_submit();

  // Submits a command to _submit_buffer buffer
  void end_one_time_submit();

  void init_materials();

  // Check if renderables need to be allocated
  void check_renderables();

  void bind_textures();
  void transfer_texture_to_gpu(Texture::Ptr texture);

  void begin_render_pass(VkRenderPass &render_pass, VkFramebuffer &framebuffer,
                         VkCommandBuffer &command_buffer,
                         const VkExtent2D &extent, float depth_clear_value,
                         float *color_clear_values,
                         int color_clear_values_count, float alpha_clear_value);
  void end_render_pass(VkCommandBuffer &command_buffer);

  void begin_command_buffer(VkCommandBuffer &command_buffer);
  void submit_command_buffer(VkCommandBuffer &command_buffer);

  void render_shadow_maps(VkCommandBuffer &command_buffer);
  void render_g_buffer(VkCommandBuffer &command_buffer);
  void render_screenspace_effects(VkCommandBuffer &command_buffer);
  void render_composite(VkCommandBuffer &command_buffer);
  void render_shading(VkCommandBuffer &command_buffer);
  void render_screenspace_smoothing(VkCommandBuffer &command_buffer);
  void render_debug(VkCommandBuffer &command_buffer);
  void render_gui(VkCommandBuffer &command_buffer);

  void begin_shadow_pass();
  void end_shadow_pass();

  void draw();

  void cleanup();

  void init_shadow_pass();

  void init_deferred_pass();

  void init_screenspace_pass();

  void init_shading_pass();

  // Debugging primitive drawing
  void draw_debug_line(const Eigen::Vector3f &start, const Eigen::Vector3f &end,
                       const Eigen::Vector3f &color);

  void draw_debug_quad(const Eigen::Matrix<float, 4, 3> &quad_verts,
                       const Eigen::Vector3f &color);

  void draw_debug_box(const Eigen::Matrix<float, 8, 4> &box_verts,
                      const Eigen::Vector3f &color);

  void draw_debug_sphere(const Eigen::Vector3f &position, float radius,
                         int resolution, const Eigen::Vector3f &color);
};
Renderer &get_renderer();
} // namespace rend