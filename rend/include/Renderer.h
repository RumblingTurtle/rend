#pragma once
#include <RenderPipelineBuilder.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <VkBootstrap.h>
#include <exception>
#include <iostream>
#include <macros.h>
#include <vector>
#include <vulkan/vulkan.h>

class Renderer {
  VkExtent2D _window_dims{500, 500};
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
  VkPipelineLayout _triangle_pipeline_layout;
  VkPipeline _triangle_pipeline;

  int frame_number = 0;

  RenderPipelineBuilder _pipeline_builder;

  Shader _triangle_vertex_shader;

  bool initialized = false;

public:
  Renderer() = default;

  ~Renderer();

  bool init();

  bool init_vulkan();

  bool init_swapchain();

  bool init_cmd_buffer();

  bool init_renderpass();

  bool init_framebuffers();

  bool init_sync_primitives();

  bool init_pipelines();

  bool draw();
};