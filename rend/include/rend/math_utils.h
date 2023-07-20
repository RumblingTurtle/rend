#pragma once
#include <Eigen/Dense>
#include <utility>

/**
 * @brief Computes min and max coordinates for a given set of vectors
 *
 * @param vertices input vertices
 * @return std::pair<Eigen::Vector3f, Eigen::Vector3f>  min,max
 */
inline std::pair<Eigen::Vector3f, Eigen::Vector3f>
compute_span(const Eigen::Matrix<float, -1, 3> &vertices) {
  std::pair<Eigen::Vector3f, Eigen::Vector3f> span;
  span.first = vertices.row(0);
  span.second = vertices.row(0);

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 3; j++) {
      if (vertices(i, j) < span.first(j)) {
        span.first(j) = vertices(i, j);
      }
      if (vertices(i, j) > span.second(j)) {
        span.second(j) = vertices(i, j);
      }
    }
  }
  return span;
}

inline Eigen::Matrix4f get_projection_matrix(float fov, float aspect,
                                             float near, float far) {
  Eigen::Matrix4f projection = Eigen::Matrix4f::Zero();
  float e = 1 / std::tan(fov / 180 * M_PI_2);
  projection(0, 0) = e / aspect;
  projection(1, 1) = e;
  projection(2, 2) = (far + near) / (near - far);
  projection(3, 3) = 0.0;
  projection(3, 2) = -1.0;
  projection(2, 3) = (2.0 * far * near) / (near - far);
  return projection;
}

inline Eigen::Matrix4f get_view_matrix(Eigen::Vector3f position,
                                       Eigen::Vector3f forward) {
  Eigen::Matrix4f model = Eigen::Matrix4f::Identity();
  Eigen::Vector3f forward_normalized = forward.normalized();
  Eigen::Vector3f right =
      Eigen::Vector3f::UnitY().cross(forward_normalized).normalized();
  Eigen::Vector3f up = forward_normalized.cross(right).normalized();

  model.block<3, 1>(0, 0) = right;
  model.block<3, 1>(0, 1) = -up;
  model.block<3, 1>(0, 2) = -forward_normalized;
  model.block<3, 1>(0, 3) = position;

  return model.inverse();
}