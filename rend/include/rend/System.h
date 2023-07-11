#pragma once
#include <rend/EntityRegistry.h>

struct System {
  virtual void update(float dt) = 0;
};