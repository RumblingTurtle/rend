#pragma once
#include <Eigen/Dense>

struct Rigidbody {
  Eigen::Vector3f linear_velocity;
  Eigen::Vector3f angular_velocity;

  Eigen::Vector3f linear_acceleration;
  Eigen::Vector3f angular_acceleration;

  float mass;
  float drag;
  float gravity;
  bool static_body;

  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  Rigidbody() : mass(1.0f), drag(0.99999f), gravity(9.8f), static_body(false) {
    linear_velocity.setZero();
    angular_velocity.setZero();
    linear_acceleration.setZero();
    angular_acceleration.setZero();
  }
};