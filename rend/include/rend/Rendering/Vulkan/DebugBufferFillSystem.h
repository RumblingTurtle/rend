#pragma once

#include <Eigen/Dense>
#include <rend/System.h>

namespace rend::systems {
/**
 * @brief Fills the Renderer debug buffer with AABBs of all entities
 *
 */
struct DebugBufferFillSystem : public System {
  void update(float dt) override {
    Renderer &renderer = rend::get_renderer();
    rend::ECS::EntityRegistry &registry = rend::ECS::get_entity_registry();

    for (rend::ECS::EntityRegistry::ArchetypeIterator rb_iterator =
             registry.archetype_iterator<Transform, AABB>();
         rb_iterator.valid(); ++rb_iterator) {
      rend::ECS::EID eid = *rb_iterator;
      Transform &transform = registry.get_component<Transform>(eid);
      AABB &aabb = registry.get_component<AABB>(eid);

      float strips[72];
      get_line_strips(aabb, strips, false);

      for (int i = 0; i < 12; i++) {
        renderer.draw_debug_line(Eigen::Vector3f::Map(strips + i * 6),
                                 Eigen::Vector3f::Map(strips + i * 6 + 3),
                                 Eigen::Vector3f(1, 0, 0));
      }

      Eigen::Matrix<float, 8, 4> vertices =
          (transform.get_model_matrix() *
           get_local_aabb_vertices(aabb).transpose())
              .transpose();
      renderer.draw_debug_quad(vertices.block<4, 3>(0, 0),
                               Eigen::Vector3f(0, 0, 1));
      renderer.draw_debug_quad(vertices.block<4, 3>(4, 0),
                               Eigen::Vector3f(0, 0, 1));
    }
  }
};
} // namespace rend::systems