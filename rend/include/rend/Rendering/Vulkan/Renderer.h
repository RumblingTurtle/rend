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

  VkExtent2D _window_dims{1000, 1000};
  SDL_Window *_window;
  VkInstance _instance;
  VkPhysicalDevice _physical_device;
  VkDevice _device;
  VkSurfaceKHR _surface;
  VkQueue _graphics_queue;
  uint32_t _queue_family;

  VkDebugUtilsMessengerEXT _debug_messenger;

  // Swapchain
  VkSwapchainKHR _swapchain;
  VkFormat _swapchain_image_format;
  std::vector<VkImage> _swapchain_images;
  std::vector<VkImageView> _swapchain_image_views;
  uint32_t _swapchain_img_idx; // Current swapchain image index

  // Depth buffer
  ImageAllocation _depth_image;

  // CMD buffer
  VkCommandPool _cmd_pool;
  VkCommandBuffer _command_buffer;

  // Renderpass
  VkRenderPass _render_pass;
  std::vector<VkFramebuffer> _framebuffers;

  // Sync primitives
  VkSemaphore _swapchain_semaphore; // Signaled when the next swapchain image
                                    // index is aquired
  VkSemaphore _render_complete_semaphore; // Signaled when rendering is done
  VkFence _command_complete_fence; // Fence for waiting on the command queue

  // VMA allocator
  VmaAllocator _allocator;

  Deallocator _deallocator;

  int _frame_number = 0;
  bool _initialized = false;

  VkDescriptorPool _descriptor_pool;

  struct {
    VkCommandPool pool;
    VkFence fence;
  } _submit_buffer;

  VkDeviceSize min_ubo_alignment;

  struct SubpassData {
    VkRenderPass render_pass;
    ImageAllocation image_allocation;
    VkSampler sampler;
    VkImageLayout layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    VkFramebuffer framebuffer;
  };

  SubpassData _shadow_pass;

  BufferAllocation _camera_buffer;
  BufferAllocation _light_buffer;

public:
  Material debug_material;
  Material geometry_material;
  Material lights_material;
  Material shadow_material;

  struct DebugVertex {
    float start[3];
    float start_color[3];
  };

  struct {
    BufferAllocation buffer;
    float debug_buffer[MAX_DEBUG_VERTICES * sizeof(DebugVertex)];
    int debug_verts_to_draw = 0;
  } debug_renderable;

  std::unique_ptr<Camera> camera;
  std::vector<LightSource> lights;

  std::unordered_map<Texture *, int> texture_to_index;

  Renderer() {
    camera = std::make_unique<Camera>(
        90.f, _window_dims.width / _window_dims.height, 0.1f, 200.0f);
    lights.resize(MAX_LIGHTS);
  };

  bool init();

  // Initializes physical and logical devices + VBA
  bool init_vulkan();

  // Allocates swapchain, it's image buffers and image views into the buffers
  bool init_swapchain();

  // Allocates the depth image and it't view
  bool init_z_buffer();

  // Initializes the command pool to be submitted to the graphics queue
  bool init_cmd_buffer();

  // Defines the renderpass with all the subpasses and their attachments
  bool init_renderpass();

  // Render passes work on framebuffers not directly on image views
  bool init_framebuffers();

  // Initializes semaphores and fences for syncronization with the gpu
  bool init_sync_primitives();

  bool init_descriptor_pool();

  bool init_debug_renderable();

  // Starts submission command buffer recording
  bool begin_one_time_submit();

  // Submits a command to _submit_buffer buffer
  bool end_one_time_submit();

  bool init_materials();

  // Check if renderables need to be allocated
  void check_renderables();

  void bind_textures();
  void transfer_texture_to_gpu(Texture::Ptr texture);

  bool begin_render_pass(VkRenderPass &render_pass, VkFramebuffer &framebuffer,
                         VkCommandBuffer &command_buffer,
                         const VkExtent2D &extent, float depth_clear_value,
                         float color_clear_value);
  void end_render_pass(VkCommandBuffer &command_buffer);

  bool begin_command_buffer(VkCommandBuffer &command_buffer);
  bool submit_command_buffer(VkCommandBuffer &command_buffer);

  void render_scene(VkCommandBuffer &command_buffer, bool shadow_pass);
  void render_debug(VkCommandBuffer &command_buffer);

  bool begin_shadow_pass();
  bool end_shadow_pass();

  bool draw();

  void cleanup();

  bool init_shadow_map();

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

static Renderer &get_renderer() {
  static Renderer renderer{};
  return renderer;
}
} // namespace rend