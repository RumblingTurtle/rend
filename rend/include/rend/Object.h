#pragma once
#include <Eigen/Dense>

// Naive implementation of entities
struct Object {
  Eigen::Vector3f position;
  Eigen::Vector3f scale;
  Eigen::Quaternionf rotation;

  Object() {
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
  Eigen::Vector3f up() const { return rotation.toRotationMatrix().col(1); }
};

class Camera : public Object {
  Eigen::Matrix4f _projection;

public:
  Camera(float fov, float aspect, float near, float far) {
    assert(aspect > 0);
    assert(far > near);
    assert(near > 0);

    _projection = Eigen::Matrix4f::Zero();
    float e = 1 / std::tan(fov / 180 * M_PI_2);
    _projection(0, 0) = e / aspect;
    _projection(1, 1) = e;
    _projection(2, 2) = (far + near) / (near - far);
    _projection(3, 3) = 0.0;
    _projection(3, 2) = -1.0;
    _projection(2, 3) = (2.0 * far * near) / (near - far);
  }

  void lookat(const Eigen::Vector3f &at) {
    Eigen::Matrix3f R;
    R.col(2) = (at - position).normalized();
    R.col(0) = Eigen::Vector3f::UnitY().cross(R.col(2)).normalized();
    R.col(1) = R.col(2).cross(R.col(0));
    rotation = Eigen::Quaternionf{R};
  }

  Eigen::Matrix4f get_view_matrix() {
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();
    view.block<3, 3>(0, 0) = rotation.toRotationMatrix();
    // Camera is looking in the negative z direction
    view.col(2) *= -1;
    view.col(1) *= -1;
    view.block<3, 1>(0, 3) = position.head<3>();
    return view.inverse();
  }

  Eigen::Matrix4f get_projection_matrix() { return _projection; }
};
