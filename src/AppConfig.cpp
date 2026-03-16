#include "AppConfig.h"

namespace {
String toUppercase(const String& value) {
  String normalized = value;
  normalized.trim();
  normalized.toUpperCase();
  return normalized;
}
}  // namespace

namespace AppDefaults {
AppConfig defaultConfig() {
  AppConfig config;
  config.schemaVersion = SCHEMA_VERSION;

  config.wifi.ssid = "";
  config.wifi.password = "";
  config.wifi.hostname = "cr-mapa";

  config.dataSource.url = "http://tmep.cz/vystup-json.php?okresy_cr=1";
  config.dataSource.parserType = "INDEXED_H1";
  config.dataSource.locationField = "name";
  config.dataSource.valueField = "h1";
  config.dataSource.colorField = "color";
  config.dataSource.minValue = -15.0f;
  config.dataSource.maxValue = 40.0f;
  config.dataSource.refreshIntervalMs = 1000;

  config.render.brightness = 10;
  config.render.wheelMin = 170;
  config.render.wheelMax = 0;

  return config;
}

ParserType parserTypeFromString(const String& parserType, ParserType fallback) {
  const String normalized = toUppercase(parserType);

  if (normalized == "INDEXED_H1") {
    return ParserType::INDEXED_H1;
  }
  if (normalized == "INDEXED_VALUE_FIELD") {
    return ParserType::INDEXED_VALUE_FIELD;
  }
  if (normalized == "NAMED_VALUE_FIELD") {
    return ParserType::NAMED_VALUE_FIELD;
  }
  if (normalized == "NAMED_COLOR_FIELD") {
    return ParserType::NAMED_COLOR_FIELD;
  }

  return fallback;
}

const char* parserTypeToString(ParserType parserType) {
  switch (parserType) {
    case ParserType::INDEXED_H1:
      return "INDEXED_H1";
    case ParserType::INDEXED_VALUE_FIELD:
      return "INDEXED_VALUE_FIELD";
    case ParserType::NAMED_VALUE_FIELD:
      return "NAMED_VALUE_FIELD";
    case ParserType::NAMED_COLOR_FIELD:
      return "NAMED_COLOR_FIELD";
    default:
      return "INDEXED_H1";
  }
}
}  // namespace AppDefaults
