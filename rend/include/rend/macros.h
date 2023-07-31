#pragma once
#include <iostream>
#include <vulkan/vulkan.h>

static void VK_CHECK(VkResult err) {
  if (err == 0) {
    return;
  }
  fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
  if (err < 0) {
    abort();
  }
}

static void VK_CHECK(VkResult err, const char *msg) {
  if (err == 0) {
    return;
  }
  fprintf(stderr, "[vulkan] Error: VkResult = %d\n %s", err, msg);
  if (err < 0) {
    abort();
  }
}

#define CLAMP(x, low, high)                                                    \
  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#define REGISTER_COMPONENT(T)                                                  \
  template void EntityRegistry::register_component<T>();                       \
  template bool EntityRegistry::is_component_registered<T>();                  \
  template int EntityRegistry::get_component_index<T>();                       \
  template bool EntityRegistry::is_component_enabled<T>(EID id);               \
  template T &EntityRegistry::add_component<T>(EID id);                        \
  template void EntityRegistry::remove_component<T>(EID id);                   \
  template T &EntityRegistry::get_component<T>(EID id);
