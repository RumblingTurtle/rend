
#pragma once
#include <any>
#include <bitset>
#include <iostream>
#include <map>
#include <memory>
#include <typeinfo>
#include <vector>

namespace ECS {

static constexpr int MAX_ENTITIES = 1000;
static constexpr int MAX_COMPONENTS = 32;

typedef uint32_t EID; // Entity ID

using TypeInfoRef = const std::type_info;

template <typename T> struct ComponentPool {
  typedef std::shared_ptr<ComponentPool<T>> Ptr;
  std::vector<T> components;
  ComponentPool() { components.reserve(MAX_ENTITIES); }
};

struct EntityRegistry {
  struct RegistryEntry {
    std::bitset<MAX_COMPONENTS + 1> mask{
        0}; // Component bitmask + 1 bit for active flag
    void reset() { mask = 0; }

    bool is_component_enabled(int index) { return mask.test(index); }
  };

  int registered_component_count = 0;
  int registered_entity_count = 0;

  std::vector<RegistryEntry> entities;
  std::vector<std::any> component_pools;
  std::map<std::size_t, int> component_indices;

  template <typename T> void register_component() {
    if (registered_component_count >= MAX_COMPONENTS) {
      throw std::runtime_error("ECS: Too many components registered");
    }

    size_t hash = typeid(T).hash_code();
    const auto &index_iterator = component_indices.find(hash);
    if (index_iterator != component_indices.end()) {
      std::cerr << "ECS: Component already registered" << std::endl;
      return;
    }
    int component_index = registered_component_count++;
    component_indices.insert({hash, component_index});
    typename ComponentPool<T>::Ptr pool = std::make_shared<ComponentPool<T>>();
    component_pools[component_index] =
        std::make_any<typename ComponentPool<T>::Ptr>(pool);
  }

  template <typename T> void add_component(EID id) {
    if (id >= MAX_ENTITIES) {
      throw std::runtime_error("ECS: Entity ID out of range");
    }

    const auto &index_iterator = component_indices.find(typeid(T).hash_code());
    if (index_iterator == component_indices.end()) {
      throw std::runtime_error(std::string("ECS: Component not registered: ") +
                               typeid(T).name());
    }

    if (entities[id].is_component_enabled(index_iterator->second)) {
      throw std::runtime_error(
          "ECS: Component already registered for entity EID " +
          std::string{id});
    }

    if (!std::any_cast<typename ComponentPool<T>::Ptr>(
            component_pools[index_iterator->second])) {
      throw std::runtime_error(
          "ECS: Component pool not initialized for component " +
          std::string{typeid(T).name()});
    }

    typename ComponentPool<T>::Ptr pool =
        std::any_cast<typename ComponentPool<T>::Ptr>(
            component_pools[index_iterator->second]);
    pool->components[id] = T{}; // Default construct component
    entities[id].mask.flip(index_iterator->second);
  }

  template <typename T> void remove_component(EID id) {
    auto &index_iterator = component_indices.find(typeid(T).hash_code());
    if (index_iterator == component_indices.end()) {
      throw std::runtime_error(std::string("ECS: Component not registered: ") +
                               typeid(T).name());
    }

    if (entities[id].is_component_enabled(index_iterator->second)) {
      std::cerr << "ECS: Component not registered for entity EID" << id
                << std::endl;
    }

    entities[id].mask.flip(index_iterator->second);
  }

  EID register_entity() {
    if (registered_entity_count >= MAX_ENTITIES) {
      throw std::runtime_error("ECS: Too many entities registered");
    }

    entities[registered_entity_count].mask.set(
        MAX_COMPONENTS); // Set active flag
    return registered_entity_count++;
  }

  template <typename T> T &get_component(EID id) {
    const auto &index_iterator = component_indices.find(typeid(T).hash_code());
    if (!entities[id].is_component_enabled(index_iterator->second)) {
      throw std::runtime_error("ECS: Component not registered for entity EID " +
                               std::string{id});
    }

    if (index_iterator == component_indices.end()) {
      throw std::runtime_error(std::string("ECS: Component not registered: ") +
                               typeid(T).name());
    }

    typename ComponentPool<T>::Ptr pool =
        std::any_cast<typename ComponentPool<T>::Ptr>(
            component_pools[index_iterator->second]);

    return pool->components[id];
  }

  static EntityRegistry &get_entity_registry() {
    static EntityRegistry registry{};
    return registry;
  };

private:
  EntityRegistry() {
    entities.resize(MAX_ENTITIES);
    component_pools.resize(MAX_COMPONENTS);
  }
};

} // namespace ECS
