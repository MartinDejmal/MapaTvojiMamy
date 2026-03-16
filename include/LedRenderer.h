#pragma once

#include <Arduino.h>

#include "AppConfig.h"
#include "DataParser.h"

class LedRenderer {
 public:
  void begin(const RenderConfig& config);
  void render(const LedState* states, size_t count, const MapProfileConfig& mapConfig);

 private:
  RenderConfig renderConfig_{};
  uint8_t wheelFromValue(float value, float minValue, float maxValue) const;
};
