#include "LedRenderer.h"

#include "AppConfig.h"
#include "Freenove_WS2812_Lib_for_ESP32.h"

namespace {
Freenove_ESP32_WS2812 strip(
    AppConfig::LED_COUNT,
    AppConfig::LED_PIN,
    AppConfig::LED_CHANNEL,
    TYPE_GRB);
}

void LedRenderer::begin() {
  strip.begin();
  strip.setBrightness(AppConfig::LED_BRIGHTNESS);
}

void LedRenderer::renderTemperatures(const int* values, size_t count) {
  const size_t ledCount = count < static_cast<size_t>(AppConfig::LED_COUNT)
                              ? count
                              : static_cast<size_t>(AppConfig::LED_COUNT);

  for (size_t i = 0; i < ledCount; ++i) {
    const int wheelValue = map(
        values[i],
        AppConfig::TEMP_MIN,
        AppConfig::TEMP_MAX,
        AppConfig::WHEEL_MIN,
        AppConfig::WHEEL_MAX);

    Serial.print(wheelValue);
    strip.setLedColorData(i, strip.Wheel(wheelValue));
  }

  strip.show();
  Serial.println();
}
