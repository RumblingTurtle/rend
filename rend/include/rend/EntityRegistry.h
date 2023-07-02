
#pragma once
#include <any>
#include <bitset>
#include <iostream>
#include <map>
#include <memory>
#include <typeinfo>
#include <unordered_set>
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
    void reset_flags() { mask = 0; }

    bool is_component_enabled(int index) { return mask.test(index); }

    bool is_entity_enabled() { return mask.test(MAX_COMPONENTS); }
  };

  int registered_component_count = 0;

  std::vector<RegistryEntry> entities;   // Entity component flags
  std::vector<std::any> component_pools; // Allocated components pools
  std::unordered_set<EID> registered_entities;
  std::map<std::size_t, int>
      component_indices; // Component type hash -> index in the pool

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

  template <typename T> int get_component_index() {
    const auto &index_iterator = component_indices.find(typeid(T).hash_code());
    if (index_iterator == component_indices.end()) {
      throw std::runtime_error(std::string("ECS: Component not registered: ") +
                               typeid(T).name());
    }
    return index_iterator->second;
  }

  template <typename T> bool is_component_enabled(EID id) {
    int component_index = get_component_index<T>();
    if (id >= MAX_ENTITIES) {
      throw std::runtime_error("ECS: Entity ID out of range");
    }
    if (!entities[id].is_entity_enabled()) {
      throw std::runtime_error("ECS: Entity not registered");
    }

    return entities[id].is_component_enabled(component_index);
  }

  template <typename T> void add_component(EID id) {
    if (id >= MAX_ENTITIES) {
      throw std::runtime_error("ECS: Entity ID out of range");
    }

    int component_index = get_component_index<T>();
    if (entities[id].is_component_enabled(component_index)) {
      throw std::runtime_error(
          "ECS: Component already registered for entity EID " +
          std::to_string(id));
    }

    if (!std::any_cast<typename ComponentPool<T>::Ptr>(
            component_pools[component_index])) {
      throw std::runtime_error(
          "ECS: Component pool not initialized for component " +
          std::string{typeid(T).name()});
    }

    typename ComponentPool<T>::Ptr pool =
        std::any_cast<typename ComponentPool<T>::Ptr>(
            component_pools[component_index]);
    pool->components[id] = T{}; // Default construct component
    entities[id].mask.flip(component_index);
  }

  template <typename T> void remove_component(EID id) {
    int component_index = get_component_index<T>();

    if (entities[id].is_component_enabled(component_index)) {
      std::cerr << "ECS: Component not registered for entity EID" << id
                << std::endl;
    }

    entities[id].mask.flip(component_index);
  }

  EID register_entity() {
    if (registered_entities.size() >= MAX_ENTITIES) {
      throw std::runtime_error("ECS: Too many entities registered");
    }
    EID new_id = get_available_id();
    registered_entities.insert(new_id);
    entities[new_id].mask.set(MAX_COMPONENTS); // Set active flag
    return new_id;
  }

  EID get_available_id() {
    for (int id = 0; id < MAX_ENTITIES; id++) {
      if (!entities[id].is_entity_enabled()) {
        return id;
      }
    }
    throw std::runtime_error(
        "ECS: No available entity IDs"); // Shouldn't happen
  }

  void remove_entity(EID id) {
    if (id >= MAX_ENTITIES) {
      throw std::runtime_error("ECS: Entity ID out of range");
    }

    for (int component_id = 0; component_id < MAX_COMPONENTS; component_id++) {
      if (entities[id].is_component_enabled(component_id)) {
        component_pools[component_id].reset();
      }
    }

    registered_entities.erase(id);
    entities[id].reset_flags();
  }

  template <typename T> T &get_component(EID id) {
    int component_index = get_component_index<T>();

    if (!entities[id].is_component_enabled(component_index)) {
      throw std::runtime_error("ECS: Component not registered for entity EID " +
                               std::to_string(id));
    }
    typename ComponentPool<T>::Ptr pool =
        std::any_cast<typename ComponentPool<T>::Ptr>(
            component_pools[component_index]);

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
