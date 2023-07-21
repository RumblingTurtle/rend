#pragma once
#include <Eigen/Dense>

#include <Jolt/Jolt.h>

#include <Jolt/Physics/Body/Body.h>

struct Rigidbody {
  enum class PrimitiveType { BOX, CAPSULE, SPHERE, CYLINDER };
  float mass;
  float damping;
  float gravity;
  bool static_body;
  JPH::BodyID body_id;
  PrimitiveType primitive_type;
  Eigen::Vector3f dimensions = Eigen::Vector3f::Ones();
  Eigen::Vector3f com_offset = Eigen::Vector3f::Zero();

  Rigidbody()
      : mass(1.0f), damping(0.99999f), gravity(9.8f), static_body(false),
        primitive_type(PrimitiveType::BOX) {
    dimensions.setOnes();
  }
};
