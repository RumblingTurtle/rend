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
#include <rend/Rendering/Vulkan/RenderPipelineBuilder.h>
#include <rend/Rendering/Vulkan/Renderable.h>
#include <rend/Rendering/Vulkan/vk_helper_types.h>
#include <rend/Rendering/Vulkan/vk_struct_init.h>
#include <rend/Transform.h>
#include <rend/macros.h>

#include <rend/EntityRegistry.h>

constexpr int MAX_DEBUG_STRIPS = 200;
constexpr int DEBUG_GRID_STRIP_COUNT = 30; // Strips per grid dimension
constexpr float DEBUG_GRID_SPAN = 500;     // Meters

class Renderer {
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
  VkCommandBuffer _cmd_buffer;

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

  std::unordered_map<Material *, std::vector<std::pair<Renderable, ECS::EID>>>
      _renderables;

  Deallocator _deallocator;

  int _frame_number = 0;
  bool _initialized = false;

  VkDescriptorPool _descriptor_pool;

  struct {
    VkCommandPool pool;
    VkCommandBuffer buffer;
    VkFence fence;
  } _submit_buffer;

  VkDeviceSize min_ubo_alignment;

public:
  struct {
    BufferAllocation buffer;
    Material material;
  } debug_renderable;

  float debug_grid_strips[DEBUG_GRID_STRIP_COUNT * 12];
  int debug_verts_to_draw = 0;

  std::unique_ptr<Camera> camera;
  std::vector<LightSource> lights;

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

  // Caches enity's renderable component + allocates mesh buffer
  bool load_renderable(ECS::EID eid);

  // Starts submission command buffer recording
  bool begin_one_time_submit();

  // Submits a command to _submit_buffer buffer
  bool end_one_time_submit();

  // Checks if all submited materials are built
  bool check_materials();

  void init_material(Material &material);
  void transfer_texture_to_gpu(Texture &texture);

  bool begin_render_pass();

  bool end_render_pass();

  bool draw();

  void cleanup();

  static Renderer &get_renderer() {
    static Renderer renderer{};
    return renderer;
  }
};