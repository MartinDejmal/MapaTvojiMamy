#pragma once

#include <Arduino.h>

class LedRenderer {
 public:
  void begin();
  void renderTemperatures(const int* values, size_t count);
};
