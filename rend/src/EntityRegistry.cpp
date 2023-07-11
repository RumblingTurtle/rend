#include <rend/EntityRegistry.h>

namespace ECS {

template <typename T> void EntityRegistry::register_component() {
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

template <typename T> int EntityRegistry::get_component_index() {
  const auto &index_iterator = component_indices.find(typeid(T).hash_code());
  if (index_iterator == component_indices.end()) {
    throw std::runtime_error(std::string("ECS: Component not registered: ") +
                             typeid(T).name());
  }
  return index_iterator->second;
}

bool EntityRegistry::is_entity_enabled(EID id) {
  return component_rows[MAX_COMPONENTS].mask.test(id);
}

bool EntityRegistry::is_component_enabled(EID id, int component_index) {
  return component_rows[component_index].mask.test(id);
}

template <typename T> bool EntityRegistry::is_component_registered() {
  return component_indices.find(typeid(T).hash_code()) !=
         component_indices.end();
}

template <typename T> bool EntityRegistry::is_component_enabled(EID id) {
  int component_index = get_component_index<T>();
  if (id >= MAX_ENTITIES) {
    throw std::runtime_error("ECS: Entity ID out of range");
  }
  if (!is_entity_enabled(id)) {
    throw std::runtime_error("ECS: Entity not registered");
  }

  return is_component_enabled(id, component_index);
}

template <typename T> T &EntityRegistry::add_component(EID id) {
  if (id >= MAX_ENTITIES) {
    throw std::runtime_error("ECS: Entity ID out of range");
  }

  int component_index = get_component_index<T>();
  if (is_component_enabled(id, component_index)) {
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
  component_rows[component_index].mask.flip(id);
  return pool->components[id];
}

template <typename T> void EntityRegistry::remove_component(EID id) {
  int component_index = get_component_index<T>();

  if (is_component_enabled(id, component_index)) {
    std::cerr << "ECS: Component not registered for entity EID" << id
              << std::endl;
  }

  component_rows[component_index].mask.flip(id);
}

EID EntityRegistry::register_entity() {
  if (registered_entities.size() >= MAX_ENTITIES) {
    throw std::runtime_error("ECS: Too many entities registered");
  }
  EID new_id = get_available_id();
  registered_entities.insert(new_id);
  component_rows[MAX_COMPONENTS].mask.flip(new_id);
  return new_id;
}

EID EntityRegistry::get_available_id() {
  for (int id = 0; id < MAX_ENTITIES; id++) {
    if (!is_entity_enabled(id)) {
      return id;
    }
  }
  throw std::runtime_error("ECS: No available entity IDs"); // Shouldn't happen
}

void EntityRegistry::remove_entity(EID id) {
  if (id >= MAX_ENTITIES) {
    throw std::runtime_error("ECS: Entity ID out of range");
  }

  for (int component_id = 0; component_id < MAX_COMPONENTS; component_id++) {
    if (is_component_enabled(id, component_id)) {
      component_pools[component_id].reset();
      component_rows[component_id].mask.reset(id);
    }
  }
  component_rows[MAX_COMPONENTS].mask.reset(id);
  registered_entities.erase(id);
}

template <typename T> T &EntityRegistry::get_component(EID id) {
  int component_index = get_component_index<T>();

  if (!is_component_enabled(id, component_index)) {
    throw std::runtime_error("ECS: Component not registered for entity EID " +
                             std::to_string(id));
  }
  typename ComponentPool<T>::Ptr pool =
      std::any_cast<typename ComponentPool<T>::Ptr>(
          component_pools[component_index]);

  return pool->components[id];
}

EntityRegistry &EntityRegistry::get_entity_registry() {
  static EntityRegistry registry{};
  return registry;
};

EntityRegistry::EntityRegistry() {
  component_rows.resize(MAX_COMPONENTS + 1);
  component_pools.resize(MAX_COMPONENTS);
}

REGISTER_COMPONENT(Transform);
REGISTER_COMPONENT(Renderable);
REGISTER_COMPONENT(AABB);
REGISTER_COMPONENT(Rigidbody);

} // namespace ECS
