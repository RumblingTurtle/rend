#include <rend/Rendering/Vulkan/Texture.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

size_t PixelBuffer::size() {
  return dims.width * dims.height * dims.depth * sizeof(unsigned char);
}

PixelBuffer::PixelBuffer(Path path) : texture_path(path), empty(false) {
  int width, height, channels;
  pixels = stbi_load(path.native().c_str(), &width, &height, &channels,
                     STBI_rgb_alpha); // Forcing alpha channel in the buffer

  dims = VkExtent3D{static_cast<uint32_t>(width), static_cast<uint32_t>(height),
                    static_cast<uint32_t>(STBI_rgb_alpha)};
}

PixelBuffer::~PixelBuffer() {
  if (pixels != nullptr) {
    stbi_image_free(pixels);
  }
}

Texture::Texture(Path path) : pixel_buffer(path) { dims = pixel_buffer.dims; }

void Texture::allocate_image(VkDevice &device, VmaAllocator &allocator,
                             Deallocator &deallocator_queue) {
  VkSamplerCreateInfo sampler_info =
      vk_struct_init::get_sampler_create_info(filter, address_mode);
  VK_CHECK(vkCreateSampler(device, &sampler_info, nullptr, &sampler),
           "Failed to create sampler");

  deallocator_queue.push([&] { vkDestroySampler(device, sampler, nullptr); });

  VkImageCreateInfo image_info = vk_struct_init::get_image_create_info(
      VK_FORMAT_R8G8B8A8_SRGB,
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VkExtent3D{dims.width, dims.height, 1}, VK_IMAGE_TYPE_2D);

  VmaAllocationCreateInfo allocinfo =
      vk_struct_init::get_allocation_info(VMA_MEMORY_USAGE_GPU_ONLY, 0);

  VkImageViewCreateInfo image_view_info =
      vk_struct_init::get_image_view_create_info(
          nullptr, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT,
          VK_IMAGE_VIEW_TYPE_2D);

  image_allocation = ImageAllocation::create(image_info, image_view_info,
                                             allocinfo, device, allocator);

  deallocator_queue.push([=] { image_allocation.destroy(); });
}