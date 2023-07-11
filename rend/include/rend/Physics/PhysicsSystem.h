#pragma once
#include <rend/EntityRegistry.h>
#include <rend/System.h>
#include <rend/components.h>

struct PhysicsSystem : public System {
  virtual void update(float dt) {
    static ECS::EntityRegistry &registry =
        ECS::EntityRegistry::get_entity_registry();

    for (ECS::EntityRegistry::ArchetypeIterator rb_iterator =
             registry.archetype_iterator<Rigidbody, Transform, AABB>();
         rb_iterator.valid(); ++rb_iterator) {
      ECS::EID eid = *rb_iterator;
      Rigidbody &rb = registry.get_component<Rigidbody>(eid);
      Transform &transform = registry.get_component<Transform>(eid);
      AABB &aabb = registry.get_component<AABB>(eid);

      { // Update global frame AABBs
        Eigen::Matrix<float, 8, 4> vertices =
            (transform.get_model_matrix() *
             get_local_aabb_vertices(aabb).transpose())
                .transpose();
        std::pair<Eigen::Vector3f, Eigen::Vector3f> span =
            compute_span(vertices.block<8, 3>(0, 0));
        aabb.min_global = span.first;
        aabb.max_global = span.second;
      }
    }

    for (ECS::EntityRegistry::ArchetypeIterator rb_iterator =
             registry.archetype_iterator<Rigidbody, Transform, AABB>();
         rb_iterator.valid(); ++rb_iterator) {
      ECS::EID eid = *rb_iterator;
      Rigidbody &rb = registry.get_component<Rigidbody>(eid);
      Transform &transform = registry.get_component<Transform>(eid);
      if (rb.static_body) {
        continue;
      }
      AABB &aabb = registry.get_component<AABB>(eid);
      bool colliding = false;
      ECS::EID colliding_eid = 0;
      for (ECS::EntityRegistry::ArchetypeIterator other_rb_iterator =
               registry.archetype_iterator<Rigidbody, Transform, AABB>();
           other_rb_iterator.valid(); ++other_rb_iterator) {
        ECS::EID eid2 = *other_rb_iterator;
        if (eid == eid2) {
          continue;
        }
        AABB &other_aabb = registry.get_component<AABB>(eid2);
        if (aabb_intersection(aabb, other_aabb)) {
          colliding = true;
          colliding_eid = eid2;
          break;
        }
      }

      if (colliding) {
        rb.linear_acceleration = Eigen::Vector3f::Zero();
        rb.linear_velocity = Eigen::Vector3f::Zero();
      } else {
        rb.linear_acceleration = Eigen::Vector3f(0.0f, -rb.gravity, 0.0f);
        rb.linear_velocity =
            rb.linear_velocity * rb.drag + rb.linear_acceleration * dt;
      }
      transform.position += rb.linear_velocity * dt;
    }
  }
};