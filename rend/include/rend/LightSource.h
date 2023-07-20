#pragma once

#include <Eigen/Dense>
#include <memory>
#include <rend/macros.h>
#include <rend/math_utils.h>

enum LightType { POINT_LIGHT, DIRECTIONAL_LIGHT };
class LightSource {
  float position[3];
  float fov;
  float color[3];
  float intensity;
  float direction[3];
  float type;
  float view_matrix[16];
  float projection_matrix[16];

public:
  void set_intensity(float intensity) { this->intensity = intensity; }
  void disable() { set_intensity(-1); }
  void enable() { set_intensity(1); }
  bool enabled() { return this->intensity > 0; }

  Eigen::Vector3f get_position() { return Eigen::Vector3f::Map(position); }
  Eigen::Vector3f get_direction() { return Eigen::Vector3f::Map(direction); }
  float get_fov() { return position[3]; }

  void set_fov(float fov) {
    position[3] = fov;
    update_VP();
  }
  void set_type(LightType type) {
    position[3] = type;
    update_VP();
  }
  void set_direction(const Eigen::Vector3f &dir) {
    Eigen::Vector3f::Map(direction) = dir;
    update_VP();
  }
  void set_position(const Eigen::Vector3f &pos) {
    Eigen::Vector3f::Map(position) = pos;
    update_VP();
  }
  void set_color(const Eigen::Vector3f &col) {
    Eigen::Vector3f::Map(color) = col;
    update_VP();
  }

  void update_VP() {
    if (enabled()) {
      Eigen::Matrix4f::Map(projection_matrix) =
          get_projection_matrix(180.0f * get_fov() / M_PI, 1, 0.1f, 500.0f);
      Eigen::Matrix4f::Map(view_matrix) =
          get_view_matrix(get_position(), get_direction());
    }
  }

  LightSource() {
    disable();
    set_fov(M_PI_2);
    set_type(DIRECTIONAL_LIGHT);
    set_direction(Eigen::Vector3f::UnitY());
    set_color(Eigen::Vector3f::Ones());
    set_position(Eigen::Vector3f::Zero());
  }
};