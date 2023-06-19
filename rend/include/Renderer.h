#pragma once
#include "vk_mem_alloc.h"
#include <Mesh.h>
#include <Object.h>
#include <RenderPipelineBuilder.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <VkBootstrap.h>
#include <deque>
#include <exception>
#include <functional>
#include <iostream>
#include <macros.h>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

class Renderer {
  VkExtent2D _window_dims{1000, 1000};
  SDL_Window *_window;
  VkInstance _instance;
  VkPhysicalDevice _physical_device;
  VkDevice _device;
  VkDebugUtilsMessengerEXT _debug_messenger;
  VkSurfaceKHR _surface;

  // Swapchain
  VkSwapchainKHR _swapchain;
  VkFormat _swapchain_image_format;

  // Swapchain images
  std::vector<VkImage> _swapchain_images;
  std::vector<VkImageView> _swapchain_image_views;
  uint32_t _swapchain_img_idx; // Current swapchain image index

  // CMD buffer
  VkCommandPool _cmd_pool;
  VkCommandBuffer _cmd_buffer;
  VkQueue _graphics_queue;
  uint32_t _queue_family;

  // Renderpass
  VkRenderPass _render_pass;
  std::vector<VkFramebuffer> _framebuffers;

  // Sync primitives
  VkSemaphore _swapchain_semaphore; // Signaled when the next swapchain image
                                    // index is aquired
  VkSemaphore _render_complete_semaphore; // Signaled when rendering is done
  VkFence _command_complete_fence; // Fence for waiting on the command queue

  // Triangle rendering layout
  std::vector<VkPipelineLayout> _pipeline_layouts;
  std::vector<VkPipeline> _pipelines;

  int frame_number = 0;

  RenderPipelineBuilder _pipeline_builder;

  bool initialized = false;

  // Vertex buffer allocator
  VmaAllocator _vb_allocator;

  // This is not the best way to organize things. But adding a proper ECS not
  // prioritized for now. A better way would be to add renderable components to
  // entities which would map to unique resource ID's which would save up memory
  std::vector<std::shared_ptr<Mesh>> _meshes;
  std::vector<std::shared_ptr<Object>> _objects;

  struct Deallocator {
    std::deque<std::function<void()>> _deletion_queue;
    void push(std::function<void()> &&function) {
      _deletion_queue.push_back(std::move(function));
    }

    ~Deallocator() {
      while (!_deletion_queue.empty()) {
        _deletion_queue.back()();
        _deletion_queue.pop_back();
      }
    }
  } _deallocator;

public:
  std::unique_ptr<Camera> _camera;

  Renderer() {
    _camera = std::make_unique<Camera>(
        90.f, _window_dims.width / _window_dims.height, 0.1f, 200.0f);
  };

  ~Renderer();

  bool init();

  bool init_vulkan();

  bool init_swapchain();

  bool init_cmd_buffer();

  bool init_renderpass();

  bool init_framebuffers();

  bool init_sync_primitives();

  bool init_pipelines();

  bool add_pipeline(Shader &shader,
                    VertexInfoDescription &vertex_info_description,
                    VkPushConstantRange &push_constant_range);

  bool load_mesh(std::shared_ptr<Mesh> p_mesh,
                 std::shared_ptr<Object> p_object);

  bool draw();
};