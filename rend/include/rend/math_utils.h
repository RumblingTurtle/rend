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