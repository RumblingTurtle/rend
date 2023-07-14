
#pragma once
#include <any>
#include <bitset>
#include <functional>
#include <iostream>
#include <memory>
#include <rend/components.h>
#include <rend/macros.h>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace rend::ECS {

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
  friend EntityRegistry &get_entity_registry();
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

  template <typename T> bool is_component_registered();
  template <typename T> void register_component();
  template <typename T> int get_component_index();
  template <typename T> bool is_component_enabled(EID id);
  template <typename T> T &add_component(EID id);
  template <typename T> void remove_component(EID id);
  template <typename T> T &get_component(EID id);

  EID register_entity();
  EID get_available_id();

  void remove_entity(EID id);

  template <typename T> std::function<bool(EID)> get_archetype_predicate() {
    return [this](EID id) {
      return is_entity_enabled(id) && is_component_enabled<T>(id);
    };
  }

  template <typename T, typename T2>
  std::function<bool(EID)> get_archetype_predicate() {
    return [this](EID id) {
      return get_archetype_predicate<T>()(id) &&
             get_archetype_predicate<T2>()(id);
    };
  }

  template <typename T, typename T2, typename T3>
  std::function<bool(EID)> get_archetype_predicate() {
    return [this](EID id) {
      return get_archetype_predicate<T>()(id) &&
             get_archetype_predicate<T2>()(id) &&
             get_archetype_predicate<T3>()(id);
    };
  }

  struct ArchetypeIterator
      : public std::iterator<std::forward_iterator_tag, EID> {
    std::function<bool(EID)> predicate;
    EID current_id = 0;
    std::unordered_set<EID>::iterator registered_iterator;
    ArchetypeIterator(std::function<bool(EID)> predicate);

    void find_next();

    EID operator==(const ArchetypeIterator &other);

    EID operator*();
    ArchetypeIterator &operator++();

    bool valid();
  };

  template <typename T, typename... Args>
  ArchetypeIterator archetype_iterator() {
    return ArchetypeIterator(get_archetype_predicate<T, Args...>());
  }

private:
  EntityRegistry();
  EntityRegistry(const EntityRegistry &) = delete;
  void operator=(EntityRegistry &&) = delete;
};

EntityRegistry &get_entity_registry();

} // namespace rend::ECS
