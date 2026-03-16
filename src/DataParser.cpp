#include "DataParser.h"

#include <ArduinoJson.h>

namespace {
uint8_t parseHexByte(const String& value) {
  return static_cast<uint8_t>(strtoul(value.c_str(), nullptr, 16));
}
}  // namespace

bool DataParser::parse(
    const String& payload,
    const AppConfig& config,
    LedState* outStates,
    size_t count) {
  clearStates(outStates, count);

  // Pole až desítek objektů + několik textových klíčů; 8 KB drží rezervu pro více formátů.
  StaticJsonDocument<8192> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print("DataParser: JSON parse failed: ");
    Serial.println(error.c_str());
    return false;
  }

  if (!doc.is<JsonArrayConst>()) {
    Serial.println("DataParser: root must be array");
    return false;
  }

  JsonArrayConst array = doc.as<JsonArrayConst>();
  const ParserType parserType = AppDefaults::parserTypeFromString(
      config.mapProfile.parserType,
      ParserType::INDEXED_H1);

  switch (parserType) {
    case ParserType::INDEXED_H1:
      return parseIndexedH1(array, outStates, count);
    case ParserType::INDEXED_VALUE_FIELD:
      return parseIndexedValueField(array, config.mapProfile.valueField, outStates, count);
    case ParserType::NAMED_VALUE_FIELD:
      return parseNamedValueField(
          array,
          config.mapProfile.locationField,
          config.mapProfile.valueField,
          outStates,
          count);
    case ParserType::NAMED_COLOR_FIELD:
      return parseNamedColorField(
          array,
          config.mapProfile.locationField,
          config.mapProfile.colorField,
          outStates,
          count);
    default:
      Serial.println("DataParser: unsupported parser type");
      return false;
  }
}

bool DataParser::parseIndexedH1(JsonArrayConst array, LedState* outStates, size_t count) const {
  size_t parsedCount = 0;
  const size_t maxCount = array.size() < count ? array.size() : count;

  for (size_t i = 0; i < maxCount; ++i) {
    JsonVariantConst value = array[i]["h1"];
    if (!value.is<float>() && !value.is<int>()) {
      continue;
    }

    outStates[i].active = true;
    outStates[i].hasNumericValue = true;
    outStates[i].numericValue = value.as<float>();
    ++parsedCount;
  }

  if (parsedCount == 0) {
    Serial.println("DataParser: INDEXED_H1 parsed no values");
    return false;
  }

  return true;
}

bool DataParser::parseIndexedValueField(
    JsonArrayConst array,
    const String& valueField,
    LedState* outStates,
    size_t count) const {
  size_t parsedCount = 0;
  const size_t maxCount = array.size() < count ? array.size() : count;

  for (size_t i = 0; i < maxCount; ++i) {
    JsonVariantConst value = array[i][valueField.c_str()];
    if (!value.is<float>() && !value.is<int>()) {
      continue;
    }

    outStates[i].active = true;
    outStates[i].hasNumericValue = true;
    outStates[i].numericValue = value.as<float>();
    ++parsedCount;
  }

  if (parsedCount == 0) {
    Serial.println("DataParser: INDEXED_VALUE_FIELD parsed no values");
    return false;
  }

  return true;
}

bool DataParser::parseNamedValueField(
    JsonArrayConst array,
    const String& locationField,
    const String& valueField,
    LedState* outStates,
    size_t count) {
  size_t parsedCount = 0;

  for (JsonVariantConst item : array) {
    const String location = String((const char*)(item[locationField.c_str()] | ""));
    if (location.isEmpty()) {
      continue;
    }

    const int ledIndex = locationRegistry_.findLedIndexByKey(location);
    if (ledIndex < 0 || static_cast<size_t>(ledIndex) >= count) {
      Serial.print("DataParser: unknown location '");
      Serial.print(location);
      Serial.println("'");
      continue;
    }

    JsonVariantConst value = item[valueField.c_str()];
    if (!value.is<float>() && !value.is<int>()) {
      continue;
    }

    outStates[ledIndex].active = true;
    outStates[ledIndex].hasNumericValue = true;
    outStates[ledIndex].numericValue = value.as<float>();
    ++parsedCount;
  }

  if (parsedCount == 0) {
    Serial.println("DataParser: NAMED_VALUE_FIELD parsed no values");
    return false;
  }

  return true;
}

bool DataParser::parseNamedColorField(
    JsonArrayConst array,
    const String& locationField,
    const String& colorField,
    LedState* outStates,
    size_t count) {
  size_t parsedCount = 0;

  for (JsonVariantConst item : array) {
    const String location = String((const char*)(item[locationField.c_str()] | ""));
    const String colorHex = String((const char*)(item[colorField.c_str()] | ""));

    if (location.isEmpty() || colorHex.isEmpty()) {
      continue;
    }

    const int ledIndex = locationRegistry_.findLedIndexByKey(location);
    if (ledIndex < 0 || static_cast<size_t>(ledIndex) >= count) {
      Serial.print("DataParser: unknown location '");
      Serial.print(location);
      Serial.println("'");
      continue;
    }

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    if (!parseHexColor(colorHex, r, g, b)) {
      Serial.print("DataParser: invalid color '");
      Serial.print(colorHex);
      Serial.println("'");
      continue;
    }

    outStates[ledIndex].active = true;
    outStates[ledIndex].hasNumericValue = false;
    outStates[ledIndex].r = r;
    outStates[ledIndex].g = g;
    outStates[ledIndex].b = b;
    ++parsedCount;
  }

  if (parsedCount == 0) {
    Serial.println("DataParser: NAMED_COLOR_FIELD parsed no values");
    return false;
  }

  return true;
}

bool DataParser::parseHexColor(const String& hex, uint8_t& r, uint8_t& g, uint8_t& b) const {
  if (hex.length() != 7 || hex[0] != '#') {
    return false;
  }

  const String rs = hex.substring(1, 3);
  const String gs = hex.substring(3, 5);
  const String bs = hex.substring(5, 7);

  r = parseHexByte(rs);
  g = parseHexByte(gs);
  b = parseHexByte(bs);
  return true;
}

void DataParser::clearStates(LedState* outStates, size_t count) const {
  for (size_t i = 0; i < count; ++i) {
    outStates[i].active = false;
    outStates[i].hasNumericValue = false;
    outStates[i].numericValue = 0.0f;
    outStates[i].r = 0;
    outStates[i].g = 0;
    outStates[i].b = 0;
  }
}
