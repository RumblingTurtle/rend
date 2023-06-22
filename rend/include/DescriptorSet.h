#pragma once
#include <macros.h>
#include <memory>
#include <types.h>
#include <vector>
#include <vk_mem_alloc.h>
#include <vk_struct_init.h>
#include <vulkan/vulkan.h>
struct Binding {
  VkShaderStageFlags stage;
  VkDescriptorType type;
  size_t size;
};

struct DescriptorSetAssembler {
  VkDescriptorSetLayout descriptor_set_layout;
  std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings;

  std::vector<VkDescriptorSet> descriptor_set;
  std::vector<Binding> bindings;

  VkDevice device;

  DescriptorSetAssembler(std::vector<Binding> bindings) : bindings(bindings) {}

  void push_descriptor(Binding binding) { bindings.push_back(binding); }

  // Creates a set with N bindings
  bool assemble_layout(VkDevice device) { // Move assembled descriptors into the
                                          // descriptor set
    this->device = device;
    for (int b_idx = 0; b_idx < bindings.size(); b_idx++) {
      VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {};
      descriptor_set_layout_binding.binding = b_idx;
      descriptor_set_layout_binding.descriptorCount =
          1; // number of elements in a single binding
      descriptor_set_layout_binding.descriptorType = bindings[b_idx].type;
      descriptor_set_layout_binding.stageFlags = bindings[b_idx].stage;
      descriptor_set_layout_binding.pImmutableSamplers = nullptr;
      descriptor_set_layout_bindings.push_back(descriptor_set_layout_binding);
    }

    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.pNext = nullptr;
    layout_info.flags = 0;
    layout_info.bindingCount = descriptor_set_layout_bindings.size();
    layout_info.pBindings = descriptor_set_layout_bindings.data();

    VK_CHECK(vkCreateDescriptorSetLayout(device, &layout_info, nullptr,
                                         &descriptor_set_layout),
             "Failed to create descriptor set layout");

    return true;
  }

  void destroy() {
    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
  }
};

struct DescriptorSetAllocator {
  std::vector<DescriptorSetAssembler> assemblers;
  VkDevice device;

public:
  std::vector<VkDescriptorSetLayout> layouts;
  std::vector<VkDescriptorSet> descriptor_sets;

  bool assemble_layouts(std::vector<std::vector<Binding>> bindings,
                        VkDevice device) {
    this->device = device;
    for (std::vector<Binding> &b : bindings) {
      assemblers.emplace_back(b);
      assemblers.back().assemble_layout(device);
      layouts.push_back(assemblers.back().descriptor_set_layout);
    }
    descriptor_sets.resize(assemblers.size());
    return true;
  }

  bool allocate_descriptor_sets(VkDescriptorPool descriptor_pool) {
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.pNext = nullptr;
    alloc_info.descriptorPool = descriptor_pool;
    alloc_info.descriptorSetCount = layouts.size();
    alloc_info.pSetLayouts = layouts.data();

    descriptor_sets.resize(layouts.size());
    return vkAllocateDescriptorSets(device, &alloc_info,
                                    descriptor_sets.data()) == VK_SUCCESS;
  }

  void bind_buffer(int set_idx, int binding_idx, BufferAllocation &allocation) {
    if (set_idx >= assemblers.size() ||
        binding_idx >= assemblers[set_idx].bindings.size()) {
      throw std::runtime_error("Binding index out of range");
    }
    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = allocation.buffer;
    buffer_info.offset = 0;
    buffer_info.range = allocation.size;

    if (assemblers[set_idx].bindings[binding_idx].type !=
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER &&
        assemblers[set_idx].bindings[binding_idx].type !=
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
      throw std::runtime_error("Binding type mismatch");
    }

    VkWriteDescriptorSet write = vk_struct_init::get_descriptor_write_info(
        binding_idx, descriptor_sets[set_idx],
        assemblers[set_idx].bindings[binding_idx].type, &buffer_info, nullptr);

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
  }

  void bind_image(int set_idx, int binding_idx, ImageAllocation &allocation,
                  VkImageLayout layout, VkSampler &sampler) {
    if (set_idx >= assemblers.size() ||
        binding_idx >= assemblers[set_idx].bindings.size()) {
      throw std::runtime_error("Binding index out of range");
    }
    VkDescriptorImageInfo image_info = {};
    image_info.imageLayout = layout;
    image_info.imageView = allocation.view;
    image_info.sampler = sampler;

    if (assemblers[set_idx].bindings[binding_idx].type !=
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER &&
        assemblers[set_idx].bindings[binding_idx].type !=
            VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE) {
      throw std::runtime_error("Binding type mismatch");
    }

    VkWriteDescriptorSet write = vk_struct_init::get_descriptor_write_info(
        binding_idx, descriptor_sets[set_idx],
        assemblers[set_idx].bindings[binding_idx].type, nullptr, &image_info);

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
  }

  void destroy_layouts() {
    for (auto &assembler : assemblers) {
      assembler.destroy();
    }
  }

  void destroy(VkDescriptorPool descriptor_pool) {
    vkFreeDescriptorSets(device, descriptor_pool, descriptor_sets.size(),
                         descriptor_sets.data());
    destroy_layouts();
  }
};