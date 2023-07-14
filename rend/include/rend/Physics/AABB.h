#pragma once
#include <Eigen/Dense>
#include <rend/Rendering/Vulkan/Mesh.h>
#include <rend/Transform.h>
#include <rend/math_utils.h>

struct AABB {
  Eigen::Vector3f min_local;
  Eigen::Vector3f max_local;
  Eigen::Vector3f min_global;
  Eigen::Vector3f max_global;

  AABB() {
    min_local.setZero();
    max_local.setZero();
    min_global.setZero();
    max_global.setZero();
  }

  AABB(const Eigen::Vector3f &min_local, const Eigen::Vector3f &max_local) {
    this->min_local = min_local;
    this->max_local = max_local;
  }

  AABB(const Mesh &mesh) {
    min_local.setZero();
    max_local.setZero();
    min_global.setZero();
    max_global.setZero();
    if (mesh.vertex_count() > 0) {
      min_local = mesh.get_vertex_pos(0);
      max_local = mesh.get_vertex_pos(0);

      // Model space coords
      for (int v_idx = 0; v_idx < mesh.vertex_count(); v_idx++) {
        Eigen::Vector3f vertex_pos = mesh.get_vertex_pos(v_idx);
        for (int i = 0; i < 3; i++) {
          if (vertex_pos(i) < min_local(i)) {
            min_local(i) = vertex_pos(i);
          }
          if (vertex_pos(i) > max_local(i)) {
            max_local(i) = vertex_pos(i);
          }
        }
      }
    }
  }
};

inline Eigen::Matrix<float, 8, 4>
get_cube_vertices(const Eigen::Vector3f &min, const Eigen::Vector3f &max) {
  // Vertices in the order:
  // (1,2,3,4)Bottom edge clockwise
  // (4,5,6,7)Top edge clockwise
  // LH coordinate system
  Eigen::Matrix<float, 8, 4> vertices;
  vertices.row(0) = Eigen::Vector4f{min(0), min(1), min(2), 1};
  vertices.row(1) = Eigen::Vector4f{max(0), min(1), min(2), 1};
  vertices.row(2) = Eigen::Vector4f{max(0), max(1), min(2), 1};
  vertices.row(3) = Eigen::Vector4f{min(0), max(1), min(2), 1};

  vertices.row(4) = Eigen::Vector4f{min(0), min(1), max(2), 1};
  vertices.row(5) = Eigen::Vector4f{max(0), min(1), max(2), 1};
  vertices.row(6) = Eigen::Vector4f{max(0), max(1), max(2), 1};
  vertices.row(7) = Eigen::Vector4f{min(0), max(1), max(2), 1};
  return vertices;
}

inline Eigen::Matrix<float, 8, 4> get_local_aabb_vertices(const AABB &aabb) {
  return get_cube_vertices(aabb.min_local, aabb.max_local);
}

inline Eigen::Matrix<float, 8, 4> get_global_aabb_vertices(const AABB &aabb) {
  return get_cube_vertices(aabb.min_global, aabb.max_global);
}

/*
  Fills in an array of strips that compose an AABB

  Top strip
  t1 ---- t2/|
  |        | |Side strip
  |        | |
  b1 ---- b2/
  Bottom strip

  NOTE: nothing really prevents you from passing
  in a smaller array so be cautious
*/
inline void get_line_strips(const AABB &aabb, float strips[72],
                            bool local = false) {
  Eigen::Matrix<float, 8, 4> vertices =
      !local ? get_global_aabb_vertices(aabb) : get_local_aabb_vertices(aabb);
  for (int edge = 0; edge < 4; edge++) {
    int b1 = edge % 4;
    int b2 = (edge + 1) % 4;

    strips[edge * 18 + 0] = vertices.row(b1)[0];
    strips[edge * 18 + 1] = vertices.row(b1)[1];
    strips[edge * 18 + 2] = vertices.row(b1)[2];

    strips[edge * 18 + 3] = vertices.row(b2)[0];
    strips[edge * 18 + 4] = vertices.row(b2)[1];
    strips[edge * 18 + 5] = vertices.row(b2)[2];

    int t1 = 4 + edge % 4;
    int t2 = 4 + (edge + 1) % 4;

    strips[edge * 18 + 6] = vertices.row(t1)[0];
    strips[edge * 18 + 7] = vertices.row(t1)[1];
    strips[edge * 18 + 8] = vertices.row(t1)[2];

    strips[edge * 18 + 9] = vertices.row(t2)[0];
    strips[edge * 18 + 10] = vertices.row(t2)[1];
    strips[edge * 18 + 11] = vertices.row(t2)[2];

    // Connecting top and bottom stips
    strips[edge * 18 + 12] = vertices.row(b1)[0];
    strips[edge * 18 + 13] = vertices.row(b1)[1];
    strips[edge * 18 + 14] = vertices.row(b1)[2];

    strips[edge * 18 + 15] = vertices.row(t1)[0];
    strips[edge * 18 + 16] = vertices.row(t1)[1];
    strips[edge * 18 + 17] = vertices.row(t1)[2];
  }
}

/**
 * @brief Checks if two AABBs intersect
 * Note that both should be in the common frame of reference
 */
inline bool aabb_intersection(AABB &aabb, AABB &aabb_other) {
  for (int i = 0; i < 3; i++) {
    if (aabb.min_global(i) > aabb_other.max_global(i) ||
        aabb.max_global(i) < aabb_other.min_global(i)) {
      return false;
    }
  }
  return true;
}