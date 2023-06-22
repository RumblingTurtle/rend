#include <Texture.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

bool Texture::load(Path path) {
  texture_path = path;
  if (!std::filesystem::exists(path)) {
    texture_path = Path(ASSET_DIRECTORY + std::string{"/textures/error.jpg"});
  }

  _pixels = stbi_load(path.native().c_str(), &width, &height, &channels,
                      STBI_rgb_alpha);

  return _pixels != nullptr;
}

bool Texture::allocate_image(VkDevice &device, VmaAllocator &allocator,
                             Deallocator &deallocator_queue) {
  VkImageCreateInfo image_info = vk_struct_init::get_image_create_info(
      VK_FORMAT_R8G8B8A8_SRGB,
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VkExtent3D{width, height, 1});

  VmaAllocationCreateInfo allocinfo =
      vk_struct_init::get_allocation_info(VMA_MEMORY_USAGE_GPU_ONLY, 0);

  VkImageViewCreateInfo image_view_info =
      vk_struct_init::get_image_view_create_info(
          nullptr, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

  image_allocation = ImageAllocation::create(image_info, image_view_info,
                                             allocinfo, device, allocator);
  deallocator_queue.push([=] { image_allocation.destroy(); });

  /*
  VkBufferCreateInfo buffer_info = {};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = width * height * channels * sizeof(unsigned char);
  buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

  // Writing buffer from CPU to GPU
  VmaAllocationCreateInfo vmaalloc_info = {};
  vmaalloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

  // Fill allocation buffer of the mesh object
  VK_CHECK(vmaCreateBuffer(allocator, &buffer_info, &vmaalloc_info,
                           &buffer_allocation.buffer,
                           &buffer_allocation.allocation, nullptr),
           "Failed to create texture image buffer");

  void *data;
  vmaMapMemory(allocator, buffer_allocation.allocation, &data);
  memcpy(data, _pixels, width * height * channels * sizeof(unsigned char));
  vmaUnmapMemory(allocator, buffer_allocation.allocation);

  deallocator_queue.push([=] {
    vmaDestroyBuffer(allocator, buffer_allocation.buffer,
                     buffer_allocation.allocation);
    vkDestroyImageView(device, buffer_allocation.image_view, nullptr);
    vmaDestroyImage(allocator, buffer_allocation.image,
                    buffer_allocation.allocation);
  });
  _buffer_generated = true;
  stbi_image_free(_pixels);
*/
  return true;
}