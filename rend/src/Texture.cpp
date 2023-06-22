#include <Texture.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

bool Texture::load(Path path) {
  texture_path = path;
  int orig_stride;
  if (!std::filesystem::exists(path)) {
    texture_path = Path(ASSET_DIRECTORY + std::string{"/textures/error.jpg"});
  }
  _raw_pixels = stbi_load(path.native().c_str(), &width, &height, &channels,
                          STBI_rgb_alpha);
  channels = STBI_rgb_alpha;

  return true;
}

bool Texture::allocate_image(VkDevice &device, VmaAllocator &allocator,
                             Deallocator &deallocator_queue) {
  VkImageCreateInfo image_info = vk_struct_init::get_image_create_info(
      VK_FORMAT_R8G8B8A8_SRGB,
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VkExtent3D{(uint32_t)width, (uint32_t)height, 1});

  VmaAllocationCreateInfo allocinfo =
      vk_struct_init::get_allocation_info(VMA_MEMORY_USAGE_GPU_ONLY, 0);

  VkImageViewCreateInfo image_view_info =
      vk_struct_init::get_image_view_create_info(
          nullptr, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

  image_allocation = ImageAllocation::create(image_info, image_view_info,
                                             allocinfo, device, allocator);

  deallocator_queue.push([=] { image_allocation.destroy(); });

  return true;
}

Texture::~Texture() { stbi_image_free(_raw_pixels); }