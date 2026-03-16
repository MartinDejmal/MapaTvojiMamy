#include "LedRenderer.h"

#include "Freenove_WS2812_Lib_for_ESP32.h"

namespace {
Freenove_ESP32_WS2812 strip(
    AppDefaults::LED_COUNT,
    AppDefaults::LED_PIN,
    AppDefaults::LED_CHANNEL,
    TYPE_GRB);

float clampf(float value, float minValue, float maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}
}  // namespace

void LedRenderer::begin(const RenderConfig& config) {
  renderConfig_ = config;

  strip.begin();
  strip.setBrightness(renderConfig_.brightness);
}

void LedRenderer::render(const LedState* states, size_t count, const MapProfileConfig& mapConfig) {
  const size_t ledCount = count < static_cast<size_t>(AppDefaults::LED_COUNT)
                              ? count
                              : static_cast<size_t>(AppDefaults::LED_COUNT);

  for (size_t i = 0; i < ledCount; ++i) {
    if (!states[i].active) {
      strip.setLedColorData(i, 0, 0, 0);
      continue;
    }

    if (states[i].hasNumericValue) {
      const uint8_t wheel =
          wheelFromValue(states[i].numericValue, mapConfig.minValue, mapConfig.maxValue);
      strip.setLedColorData(i, strip.Wheel(wheel));
      continue;
    }

    strip.setLedColorData(i, states[i].r, states[i].g, states[i].b);
  }

  strip.show();
}

uint8_t LedRenderer::wheelFromValue(float value, float minValue, float maxValue) const {
  if (maxValue <= minValue) {
    return static_cast<uint8_t>(renderConfig_.wheelMin);
  }

  const float normalized =
      (clampf(value, minValue, maxValue) - minValue) / (maxValue - minValue);

  const float wheel =
      static_cast<float>(renderConfig_.wheelMin) +
      normalized * static_cast<float>(renderConfig_.wheelMax - renderConfig_.wheelMin);

  int clamped = static_cast<int>(wheel);
  if (clamped < 0) {
    clamped = 0;
  }
  if (clamped > 255) {
    clamped = 255;
  }

  return static_cast<uint8_t>(clamped);
}
