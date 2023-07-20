#pragma once
#include <Eigen/Dense>
#include <rend/math_utils.h>

// Naive implementation of entities
struct Transform {
  Eigen::Vector3f position;
  Eigen::Vector3f scale;
  Eigen::Quaternionf rotation;
  // Pad or add?

  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  Transform() {
    position = Eigen::Vector3f::Zero();
    scale = Eigen::Vector3f::Ones();
    rotation = Eigen::Quaternionf::Identity();
  }

  Eigen::Matrix4f get_model_matrix() const {
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();
    model.block<3, 3>(0, 0) = rotation.toRotationMatrix() * scale.asDiagonal();
    model.block<3, 1>(0, 3) = position.head<3>();
    return model;
  }

  // Using quaternions handles basis orthogonality
  Eigen::Vector3f forward() const { return rotation.toRotationMatrix().col(2); }
  Eigen::Vector3f right() const { return rotation.toRotationMatrix().col(0); }

  void lookat(const Eigen::Vector3f &at) {
    Eigen::Matrix3f R;
    R.col(2) = (at - position).normalized();
    R.col(0) = Eigen::Vector3f::UnitY().cross(R.col(2)).normalized();
    R.col(1) = R.col(2).cross(R.col(0));
    rotation = Eigen::Quaternionf{R};
  }

  Eigen::Matrix4f get_view_matrix() const {
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();
    view.block<3, 3>(0, 0) = rotation.toRotationMatrix();
    // Camera is looking in the negative z direction
    view.col(2) *= -1;
    view.col(1) *= -1;
    view.block<3, 1>(0, 3) = position.head<3>();
    return view.inverse();
  }
};

struct Camera : public Transform {
  Eigen::Matrix4f projection;
  float near, far;
  float fov;

public:
  Camera(float fov, float aspect, float near, float far)
      : near(near), far(far), fov(fov) {
    projection = get_projection_matrix(fov, aspect, near, far);
  }
};
