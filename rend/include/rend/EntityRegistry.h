
#pragma once
#include <any>
#include <bitset>
#include <iostream>
#include <memory>
#include <rend/components.h>
#include <rend/macros.h>
#include <typeinfo>
#include <unordered_map>
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
  ComponentPool() { components.resize(MAX_ENTITIES); }
};

struct EntityRegistry {
  struct RegistryEntry {
    std::bitset<MAX_ENTITIES> mask{0}; // Entity row for a single component
    bool is_component_enabled(EID entity_id) { return mask.test(entity_id); }
  };

  bool initialized = false;
  int registered_component_count = 0;

  std::vector<RegistryEntry> component_rows; // Component rows for all entities
  std::vector<std::any> component_pools;     // Allocated components pools
  std::unordered_set<EID> registered_entities;
  std::unordered_map<std::size_t, int>
      component_indices; // Component type hash -> index in the pool

  bool is_entity_enabled(EID id);
  bool is_component_enabled(EID id, int component_index);

  template <typename T> void register_component();
  template <typename T> int get_component_index();
  template <typename T> bool is_component_enabled(EID id);
  template <typename T> void add_component(EID id);
  template <typename T> void remove_component(EID id);
  template <typename T> T &get_component(EID id);

  EID register_entity();
  EID get_available_id();

  void remove_entity(EID id);

  static EntityRegistry &get_entity_registry();

  /*
  template <typename T> struct EntityIterator {
    EntityRegistry &registry;
    int component_index = get_component_index<T>();
    EID current_id = 0;

    EntityIterator(EntityRegistry &registry) : registry(registry) {}

    EID next() {

      while (current_id < MAX_ENTITIES) {
        if (registry.entities[current_id].is_entity_enabled()) {
          return current_id++;
        }
        current_id++;
      }
      return MAX_ENTITIES;
    }
  };
  */
private:
  EntityRegistry();
  EntityRegistry(const EntityRegistry &) = delete;
  void operator=(EntityRegistry &&) = delete;
};

extern EntityRegistry entity_registry;
} // namespace ECS
