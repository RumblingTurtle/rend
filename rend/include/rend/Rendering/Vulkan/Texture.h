#pragma once
#include <cstring>
#include <rend/Rendering/Vulkan/vk_helper_types.h>
#include <rend/Rendering/Vulkan/vk_struct_init.h>
#include <rend/macros.h>
#include <stb_image.h>
#include <vector>
#include <vk_mem_alloc.h>
// Loaded texture data
struct PixelBuffer {
  Path texture_path;
  std::vector<unsigned char> pixels;
  VkExtent3D dims; // Width, height, depth
  bool empty = true;

  size_t size(); // Size in bytes

  PixelBuffer() = default;
  PixelBuffer(Path path);
  PixelBuffer(std::vector<unsigned char> pixels, int width, int height);
};

struct Texture {
  typedef std::shared_ptr<Texture> Ptr;

  PixelBuffer pixel_buffer;

  ImageAllocation image_allocation;
  VkSampler sampler;

  VkFilter filter = VK_FILTER_LINEAR;
  VkSamplerAddressMode address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  Texture(Path path);
  Texture(PixelBuffer &pixel_buffer);

  void allocate_image(VkDevice &device, VmaAllocator &allocator,
                      Deallocator &deallocator_queue);

  bool image_allocated() { return image_allocation.buffer_allocated; }

  static Texture::Ptr get_error_texture() {
    static Texture::Ptr error_texture = std::make_shared<Texture>(
        Path(ASSET_DIRECTORY + std::string{"/textures/error.jpg"}));
    return error_texture;
  }
};
