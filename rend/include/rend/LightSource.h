#pragma once

#include <Eigen/Dense>
#include <memory>
#include <rend/macros.h>

class LightSource {
public:
  Eigen::Vector3f position;
  alignas(16) Eigen::Vector4f color;

  void disable() { color.w() = -1; }
  void enable() { color.w() = 1; }

  LightSource()
      : position(Eigen::Vector3f::Zero()), color(Eigen::Vector4f::Zero()) {
    disable();
  }

  LightSource(Eigen::Vector3f position, Eigen::Vector3f color, float intensity)
      : position(position) {
    this->color.head(3) = color;
    this->color.w() = intensity;
  }
};