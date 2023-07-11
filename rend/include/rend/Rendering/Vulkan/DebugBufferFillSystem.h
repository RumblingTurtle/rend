#pragma once

#include <Eigen/Dense>
#include <rend/System.h>

/**
 * @brief Fills the Renderer debug buffer with AABBs of all entities
 *
 */
struct DebugBufferFillSystem : public System {
  void update(float dt) override {
    Renderer &renderer = Renderer::get_renderer();
    ECS::EntityRegistry &registry = ECS::EntityRegistry::get_entity_registry();

    int debug_buffer_offset = sizeof(renderer.debug_grid_strips);
    renderer.debug_verts_to_draw = 0;
    for (ECS::EntityRegistry::ArchetypeIterator rb_iterator =
             registry.archetype_iterator<Transform, AABB>();
         rb_iterator.valid(); ++rb_iterator) {
      ECS::EID eid = *rb_iterator;
      Transform &transform = registry.get_component<Transform>(eid);
      AABB &aabb = registry.get_component<AABB>(eid);

      float strips[72];
      get_line_strips(aabb, strips, false);

      renderer.debug_renderable.buffer.copy_from(strips, sizeof(strips),
                                                 debug_buffer_offset);
      debug_buffer_offset += sizeof(strips);
      renderer.debug_verts_to_draw += 24;
    }
  }
};