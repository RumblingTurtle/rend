#pragma once

#include <Eigen/Dense>
#include <rend/Physics/AABB.h>
#include <rend/Physics/Rigidbody.h>
#include <rend/System.h>
#include <rend/Transform.h>

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
             registry.archetype_iterator<Transform, Rigidbody>();
         rb_iterator.valid(); ++rb_iterator) {
      rend::ECS::EID eid = *rb_iterator;
      Transform &transform = registry.get_component<Transform>(eid);
      Rigidbody &rigidbody = registry.get_component<Rigidbody>(eid);

      if (rigidbody.primitive_type == Rigidbody::PrimitiveType::BOX &&
          registry.is_component_enabled<AABB>(eid)) {
        AABB &aabb = registry.get_component<AABB>(eid);
        Eigen::Matrix<float, 8, 4> aabb_vertices =
            get_global_aabb_vertices(aabb);
        Eigen::Matrix<float, 8, 4> model_transform_vertices =
            (transform.get_model_matrix() *
             get_local_aabb_vertices(aabb).transpose())
                .transpose();

        renderer.draw_debug_box(aabb_vertices, Eigen::Vector3f(1, 0, 0));
        renderer.draw_debug_box(model_transform_vertices,
                                Eigen::Vector3f(0, 1, 0));
      } else if (rigidbody.primitive_type == Rigidbody::PrimitiveType::SPHERE) {
        renderer.draw_debug_sphere(transform.position,
                                   rigidbody.dimensions[0] *
                                       1.04, // Extends the sphere a bit
                                   20, Eigen::Vector3f(0, 0, 1));
      }
    }
  }
};
} // namespace rend::systems