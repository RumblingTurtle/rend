#pragma once
#include <iostream>

#define VK_CHECK(x, err)                                                       \
  if (x != VK_SUCCESS && x != VK_TIMEOUT) {                                    \
    std::cerr << err << std::endl << x << std::endl;                           \
    return false;                                                              \
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
