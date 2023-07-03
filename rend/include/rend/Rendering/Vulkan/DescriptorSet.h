#pragma once
#include <memory>
#include <rend/Rendering/Vulkan/vk_helper_types.h>
#include <rend/Rendering/Vulkan/vk_struct_init.h>
#include <rend/macros.h>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
struct Binding {
  VkShaderStageFlags stage;
  VkDescriptorType type;
  size_t size;
  size_t descriptor_count;
};

struct DescriptorSetAssembler {
  VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
  std::vector<Binding> bindings;
  VkDevice device;

  // Creates a set with N bindings
  bool assemble_layout(std::vector<Binding> &bindings,
                       VkDevice device) { // Move assembled descriptors into the
                                          // descriptor set
    std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings;
    this->bindings = bindings;
    this->device = device;
    for (int b_idx = 0; b_idx < bindings.size(); b_idx++) {
      VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {};
      descriptor_set_layout_binding.binding = b_idx;
      descriptor_set_layout_binding.descriptorCount =
          bindings[b_idx]
              .descriptor_count; // number of elements in a single binding
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

  std::vector<VkDescriptorSetLayout> layouts;
  std::vector<VkDescriptorSet> descriptor_sets;

  void init(std::vector<std::vector<Binding>> &bindings, VkDevice device,
            VkDescriptorPool descriptor_pool) {
    this->device = device;
    assemblers.resize(bindings.size());
    layouts.resize(bindings.size(), VK_NULL_HANDLE);
    descriptor_sets.resize(bindings.size(), VK_NULL_HANDLE);
    for (int set_idx = 0; set_idx < bindings.size(); set_idx++) {
      assemblers[set_idx].assemble_layout(bindings[set_idx], device);
      layouts[set_idx] = assemblers[set_idx].descriptor_set_layout;
    }
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.pNext = nullptr;
    alloc_info.descriptorPool = descriptor_pool;
    alloc_info.descriptorSetCount = layouts.size();
    alloc_info.pSetLayouts = layouts.data();
    if (vkAllocateDescriptorSets(device, &alloc_info, descriptor_sets.data()) !=
        VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate descriptor sets");
    }
  }

  void bind_buffer(int set_idx, int binding_idx, BufferAllocation &allocation) {
    if (set_idx >= assemblers.size() ||
        binding_idx >= assemblers[set_idx].bindings.size()) {
      throw std::runtime_error("Binding index out of range");
    }

    int descriptor_count =
        assemblers[set_idx].bindings[binding_idx].descriptor_count;
    int descriptor_size = assemblers[set_idx].bindings[binding_idx].size;
    std::vector<VkDescriptorBufferInfo> buffer_infos;
    for (int d = 0; d < descriptor_count; d++) {
      VkDescriptorBufferInfo buffer_info = {};
      buffer_info.buffer = allocation.buffer;
      buffer_info.offset = d * descriptor_size;
      buffer_info.range = descriptor_size;
      buffer_infos.push_back(buffer_info);
    }

    if (assemblers[set_idx].bindings[binding_idx].type !=
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER &&
        assemblers[set_idx].bindings[binding_idx].type !=
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
      throw std::runtime_error("Binding type mismatch");
    }

    VkWriteDescriptorSet write = vk_struct_init::get_descriptor_write_info(
        binding_idx, descriptor_sets[set_idx],
        assemblers[set_idx].bindings[binding_idx].type, descriptor_count,
        buffer_infos.data(), nullptr);

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
        assemblers[set_idx].bindings[binding_idx].type, 1, nullptr,
        &image_info);

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
  }

  void destroy(VkDescriptorPool descriptor_pool) {
    vkFreeDescriptorSets(device, descriptor_pool, descriptor_sets.size(),
                         descriptor_sets.data());
    for (auto &assembler : assemblers) {
      assembler.destroy();
    }
  }
};