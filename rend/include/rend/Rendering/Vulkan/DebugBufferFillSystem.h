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

    for (LightSource &light : renderer.lights) {
      if (!light.enabled()) {
        continue;
      }
      Eigen::Vector3f light_pos = light.get_position();
      Eigen::Matrix4f light_view =
          get_view_matrix(light_pos, light.get_direction()).inverse();

      Eigen::Vector3f up = (light_view * Eigen::Vector4f{0, 1, 0, 1}).head<3>();
      Eigen::Vector3f right =
          (light_view * Eigen::Vector4f{1, 0, 0, 1}).head<3>();
      Eigen::Vector3f forward =
          (light_view * Eigen::Vector4f{0, 0, 1, 1}).head<3>();

      renderer.draw_debug_line(light_pos, right, Eigen::Vector3f(1, 0, 0));
      renderer.draw_debug_line(light_pos, forward, Eigen::Vector3f(0, 0, 1));
      renderer.draw_debug_line(light_pos, up, Eigen::Vector3f(0, 1, 0));
    }
    // Origin gizmo
    {
      renderer.draw_debug_line(Eigen::Vector3f::Zero(),
                               Eigen::Vector3f::UnitZ() * 5,
                               Eigen::Vector3f(0, 0, 1));
      renderer.draw_debug_line(Eigen::Vector3f::Zero(),
                               Eigen::Vector3f::UnitY() * 5,
                               Eigen::Vector3f(0, 1, 0));
      renderer.draw_debug_line(Eigen::Vector3f::Zero(),
                               Eigen::Vector3f::UnitX() * 5,
                               Eigen::Vector3f(1, 0, 0));
    }
  }
};
} // namespace rend::systems