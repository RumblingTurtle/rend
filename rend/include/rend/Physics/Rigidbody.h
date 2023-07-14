#pragma once
#include <Eigen/Dense>

#include <Jolt/Jolt.h>

#include <Jolt/Physics/Body/Body.h>

struct Rigidbody {
  float mass;
  float damping;
  float gravity;
  bool static_body;
  JPH::BodyID body_id;

  Rigidbody()
      : mass(1.0f), damping(0.99999f), gravity(9.8f), static_body(false) {}
};
