#include "LedRenderer.h"

#include <FastLED.h>

namespace {
CRGB leds[AppDefaults::LED_COUNT];

// Mapování TMEP okresů (index 0..76 => TMEP ID 1..77) na fyzické LED pořadí
// LaskaKit. Hodnota -1 znamená, že pro daný okres není LED k dispozici.
constexpr int kLaskaKitOrder[AppDefaults::LED_COUNT] = {
    24, 19, 16, 10, 15, 9, 6, 8, 3, 0, 4, 1, 2, 5, 11, 7, 12, 18, 21, 29, 27, 34, 38, 31, 17, 25,
    45, 30, 36, 35, 42, 44, 56, 48, 61, 57, 65, 68, 59, 49, 55, 63, -1, 71, 69, 62, 47, 53, 46, 51,
    64, 52, 67, 70, 66, 60, 58, 54, 50, 37, -1, 40, -1, 41, 23, 22, 14, 13, 20, 28, 33, 39, 43, 32,
    -1, -1, 26,
};

float clampf(float value, float minValue, float maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

// jednoduchá náhrada původního Wheel() mapování
CRGB colorFromWheel(uint8_t pos) {
  pos = 255 - pos;
  if (pos < 85) {
    return CRGB(255 - pos * 3, 0, pos * 3);
  }
  if (pos < 170) {
    pos -= 85;
    return CRGB(0, pos * 3, 255 - pos * 3);
  }
  pos -= 170;
  return CRGB(pos * 3, 255 - pos * 3, 0);
}
}  // namespace

void LedRenderer::begin(const RenderConfig& config) {
  renderConfig_ = config;
  FastLED.addLeds<WS2812, AppDefaults::LED_PIN, GRB>(leds, AppDefaults::LED_COUNT);
  FastLED.setBrightness(renderConfig_.brightness);
  FastLED.clear(true);
}

void LedRenderer::playStartupAnimation() {
  constexpr unsigned long kWhiteFadeDurationMs = 3000;
  constexpr unsigned long kRainbowDurationMs = 3000;
  constexpr unsigned long kFrameDelayMs = 16;
  const unsigned long startMs = millis();

  while (true) {
    const unsigned long now = millis();
    const unsigned long elapsed = now - startMs;
    if (elapsed >= kWhiteFadeDurationMs) {
      break;
    }

    const float progress = static_cast<float>(elapsed) / static_cast<float>(kWhiteFadeDurationMs);
    const float litTarget = progress * static_cast<float>(AppDefaults::LED_COUNT);

    for (int i = 0; i < AppDefaults::LED_COUNT; ++i) {
      leds[i] = (static_cast<float>(i) < litTarget) ? CRGB::White : CRGB::Black;
    }
    FastLED.show();
    delay(kFrameDelayMs);
  }

  for (int i = 0; i < AppDefaults::LED_COUNT; ++i) {
    leds[i] = CRGB::White;
  }
  FastLED.show();

  const unsigned long rainbowStartMs = millis();
  while (true) {
    const unsigned long now = millis();
    const unsigned long elapsed = now - rainbowStartMs;
    if (elapsed >= kRainbowDurationMs) {
      break;
    }

    const uint8_t phase = static_cast<uint8_t>((elapsed * 255UL) / kRainbowDurationMs);
    for (int i = 0; i < AppDefaults::LED_COUNT; ++i) {
      const uint8_t wheelPos =
          static_cast<uint8_t>(phase + ((i * 256U) / AppDefaults::LED_COUNT));
      leds[i] = colorFromWheel(wheelPos);
    }
    FastLED.show();
    delay(kFrameDelayMs);
  }
}

void LedRenderer::render(const LedState* states, size_t count, const MapProfileConfig& mapConfig) {
  const size_t ledCount = count < static_cast<size_t>(AppDefaults::LED_COUNT)
                              ? count
                              : static_cast<size_t>(AppDefaults::LED_COUNT);

  FastLED.clear(false);

  for (size_t i = 0; i < ledCount; ++i) {
    const int physicalIndex = mapLogicalToPhysicalIndex(i);
    if (physicalIndex < 0 || physicalIndex >= AppDefaults::LED_COUNT) {
      continue;
    }

    if (!states[i].active) {
      leds[physicalIndex] = CRGB::Black;
      continue;
    }

    if (states[i].hasNumericValue) {
      const uint8_t wheel = wheelFromValue(states[i].numericValue, mapConfig.minValue, mapConfig.maxValue);
      leds[physicalIndex] = colorFromWheel(wheel);
      continue;
    }

    leds[physicalIndex] = CRGB(states[i].r, states[i].g, states[i].b);
  }

  FastLED.show();
}

int LedRenderer::mapLogicalToPhysicalIndex(size_t logicalIndex) const {
  if (logicalIndex >= static_cast<size_t>(AppDefaults::LED_COUNT)) {
    return -1;
  }

  if (renderConfig_.ledOrder == "LASKAKIT") {
    return kLaskaKitOrder[logicalIndex];
  }

  return static_cast<int>(logicalIndex);
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
  if (clamped < 0) clamped = 0;
  if (clamped > 255) clamped = 255;

  return static_cast<uint8_t>(clamped);
}
