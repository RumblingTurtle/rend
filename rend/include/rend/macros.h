#pragma once
#include <iostream>

#define VK_CHECK(x, err)                                                       \
  if (x != VK_SUCCESS && x != VK_TIMEOUT) {                                    \
    std::cerr << err << std::endl << x << std::endl;                           \
    return false;                                                              \
  }

#define CLAMP(x, low, high)                                                    \
  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))