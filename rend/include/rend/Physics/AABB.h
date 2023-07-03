#pragma once
#include <Eigen/Dense>
#include <rend/Rendering/Vulkan/Mesh.h>

struct AABB {
  Eigen::Vector3f min;
  Eigen::Vector3f max;
  bool in_world_frame = false;

  AABB() {
    min.setZero();
    max.setZero();
  }

  void compute(Eigen::Vector3f min, Eigen::Vector3f max) {
    this->min = min;
    this->max = max;
  }

  void compute(Mesh &mesh, const Object &object) {
    // Model space coords
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

  Eigen::Matrix<float, 8, 4> get_vertices() {
    // Vertices in model space in the order:
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

  // Returns AABB in frame of the provided object
  AABB compute_world_frame(const Object &object) {
    if (in_world_frame) {
      return *this;
    }

    AABB world_frame_aabb = *this;
    Eigen::Matrix<float, 8, 4> vertices =
        (object.get_model_matrix() * get_vertices().transpose()).transpose();
    world_frame_aabb.min = vertices.row(0).head<3>();
    world_frame_aabb.max = vertices.row(0).head<3>();
    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 3; j++) {
        if (vertices(i, j) < world_frame_aabb.min(j)) {
          world_frame_aabb.min(j) = vertices(i, j);
        }
        if (vertices(i, j) > world_frame_aabb.max(j)) {
          world_frame_aabb.max(j) = vertices(i, j);
        }
      }
    }
    world_frame_aabb.in_world_frame = true;
    return world_frame_aabb;
  }

  // NOTE: nothing really prevents you from passing
  // in a smaller array so be cautious
  void get_line_strips(float strips[72]) {
    Eigen::Matrix<float, 8, 4> vertices = get_vertices();
    for (int edge = 0; edge < 4; edge++) {
      /**
       * Top strip
       * t1 ---- t2/|
       * |        | |Side strip
       * |        | |
       * b1 ---- b2/
       * Bottom strip
       */
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

  // other_rel_pos: position of other AABB in current AABB frame of
  // reference
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