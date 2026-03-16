#pragma once

#include <Arduino.h>

enum class ParserType {
  INDEXED_H1,
  INDEXED_VALUE_FIELD,
  NAMED_VALUE_FIELD,
  NAMED_COLOR_FIELD,
};

struct WifiConfig {
  String ssid;
  String password;
  String hostname;
};

struct MapProfileConfig {
  String url;
  String parserType;
  String locationField;
  String valueField;
  String colorField;
  float minValue;
  float maxValue;
  uint32_t refreshIntervalMs;
};

struct RenderConfig {
  uint8_t brightness;
  int wheelMin;
  int wheelMax;
};

struct AppConfig {
  uint16_t schemaVersion;
  WifiConfig wifi;
  MapProfileConfig dataSource;
  RenderConfig render;
};

namespace AppDefaults {
constexpr int LED_COUNT = 77;
constexpr int LED_PIN = 27;
constexpr int LED_CHANNEL = 0;
constexpr uint16_t SCHEMA_VERSION = 1;

AppConfig defaultConfig();

ParserType parserTypeFromString(const String& parserType, ParserType fallback);
const char* parserTypeToString(ParserType parserType);
}  // namespace AppDefaults
