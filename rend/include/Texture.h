#pragma once
#include <cstring>
#include <macros.h>
#include <types.h>
#include <vector>
#include <vk_mem_alloc.h>
#include <vk_struct_init.h>

class Texture {
  unsigned char *_pixels;
  bool _buffer_generated = false;

public:
  Path texture_path;
  int width, height, channels;

  ImageAllocation image_allocation;

  bool load(Path path);
  bool allocate_image(VkDevice &device, VmaAllocator &allocator,
                      Deallocator &deallocator_queue);
  size_t get_pixels_size() {
    return width * height * channels * sizeof(unsigned char);
  }
};