#pragma once
#include <Eigen/Dense>
#include <rend/Rendering/Vulkan/Mesh.h>

struct AABB {
  // Model space coords
  Eigen::Vector3f min;
  Eigen::Vector3f max;

  AABB() {
    min.setZero();
    max.setZero();
  }

  AABB(Eigen::Vector3f min, Eigen::Vector3f max) : min(min), max(max) {}
  AABB(Mesh &mesh) {
    for (int v_idx = 0; v_idx < mesh.vertex_count(); v_idx++) {
      Eigen::Vector3f vertex_pos = mesh.get_vertex_pos(v_idx);
      for (int i = 0; i < 3; i++) {
        if (vertex_pos(i) < min(i)) {
          min(i) = vertex_pos(i);
        }
        if (vertex_pos(i) > max(i)) {
          max(i) = vertex_pos(i);
        }
      }
    }
  }

  // other_rel_pos: position of other AABB in current AABB frame of reference
  bool intersects(AABB &other, const Eigen::Vector3f &other_rel_pos) {
    Eigen::Vector3f other_min = other.min + other_rel_pos;
    Eigen::Vector3f other_max = other.max + other_rel_pos;

    for (int i = 0; i < 3; i++) {
      if (min(i) > other.max(i) || max(i) < other.min(i)) {
        return false;
      }
    }
    return true;
  }
};