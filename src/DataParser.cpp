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
    size_t count,
    ParseStats* outStats) {
  clearStates(outStates, count);
  resetStats(outStats);

  // Strip UTF-8 BOM (EF BB BF) if present at the beginning of the payload
  const char* jsonStart = payload.c_str();
  if (payload.length() >= 3 &&
      static_cast<uint8_t>(jsonStart[0]) == 0xEF &&
      static_cast<uint8_t>(jsonStart[1]) == 0xBB &&
      static_cast<uint8_t>(jsonStart[2]) == 0xBF) {
    jsonStart += 3;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonStart);
  if (error) {
    Serial.print("DataParser: JSON parse failed: ");
    Serial.println(error.c_str());
    if (outStats != nullptr) {
      outStats->error = String("JSON parse failed: ") + error.c_str();
    }
    return false;
  }

  const ParserType parserType = AppDefaults::parserTypeFromString(
      config.mapProfile.parserType,
      ParserType::INDEXED_H1);

  switch (parserType) {
    case ParserType::INDEXED_H1:
    case ParserType::INDEXED_VALUE_FIELD:
    case ParserType::NAMED_VALUE_FIELD:
    case ParserType::NAMED_COLOR_FIELD: {
      if (!doc.is<JsonArrayConst>()) {
        Serial.println("DataParser: root must be array");
        if (outStats != nullptr) {
          outStats->error = "Root JSON must be array";
        }
        return false;
      }

      JsonArrayConst array = doc.as<JsonArrayConst>();
      if (parserType == ParserType::INDEXED_H1) {
        return parseIndexedH1(array, outStates, count, outStats);
      }
      if (parserType == ParserType::INDEXED_VALUE_FIELD) {
        return parseIndexedValueField(array, config.mapProfile.valueField, outStates, count, outStats);
      }
      if (parserType == ParserType::NAMED_VALUE_FIELD) {
        return parseNamedValueField(
            array,
            config.mapProfile.locationField,
            config.mapProfile.valueField,
            outStates,
            count,
            outStats);
      }
      return parseNamedColorField(
          array,
          config.mapProfile.locationField,
          config.mapProfile.colorField,
          outStates,
          count,
          outStats);
    }
    case ParserType::OBJECT_LIST_ID_RGB: {
      if (!doc.is<JsonObjectConst>()) {
        Serial.println("DataParser: root must be object");
        if (outStats != nullptr) {
          outStats->error = "Root JSON must be object";
        }
        return false;
      }

      JsonObjectConst rootObject = doc.as<JsonObjectConst>();
      return parseObjectListIdRgb(rootObject, outStates, count, outStats);
    }
    default:
      Serial.println("DataParser: unsupported parser type");
      if (outStats != nullptr) {
        outStats->error = "Unsupported parser type";
      }
      return false;
  }
}

bool DataParser::parseIndexedH1(
    JsonArrayConst array,
    LedState* outStates,
    size_t count,
    ParseStats* outStats) const {
  int parsedCount = 0;
  int ignoredCount = 0;
  const size_t maxCount = array.size() < count ? array.size() : count;

  for (size_t i = 0; i < maxCount; ++i) {
    JsonVariantConst value = array[i]["h1"];
    if (!value.is<float>() && !value.is<int>()) {
      ++ignoredCount;
      continue;
    }

    outStates[i].active = true;
    outStates[i].hasNumericValue = true;
    outStates[i].numericValue = value.as<float>();
    ++parsedCount;
  }

  if (outStats != nullptr) {
    outStats->recognizedCount = parsedCount;
    outStats->unknownCount = ignoredCount;
    outStats->activeCount = parsedCount;
  }

  if (parsedCount == 0) {
    Serial.println("DataParser: INDEXED_H1 parsed no values");
    if (outStats != nullptr) {
      outStats->error = "INDEXED_H1 parsed no values";
    }
    return false;
  }

  return true;
}

bool DataParser::parseIndexedValueField(
    JsonArrayConst array,
    const String& valueField,
    LedState* outStates,
    size_t count,
    ParseStats* outStats) const {
  int parsedCount = 0;
  int ignoredCount = 0;
  const size_t maxCount = array.size() < count ? array.size() : count;

  for (size_t i = 0; i < maxCount; ++i) {
    JsonVariantConst value = array[i][valueField.c_str()];
    if (!value.is<float>() && !value.is<int>()) {
      ++ignoredCount;
      continue;
    }

    outStates[i].active = true;
    outStates[i].hasNumericValue = true;
    outStates[i].numericValue = value.as<float>();
    ++parsedCount;
  }

  if (outStats != nullptr) {
    outStats->recognizedCount = parsedCount;
    outStats->unknownCount = ignoredCount;
    outStats->activeCount = parsedCount;
  }

  if (parsedCount == 0) {
    Serial.println("DataParser: INDEXED_VALUE_FIELD parsed no values");
    if (outStats != nullptr) {
      outStats->error = "INDEXED_VALUE_FIELD parsed no values";
    }
    return false;
  }

  return true;
}

bool DataParser::parseNamedValueField(
    JsonArrayConst array,
    const String& locationField,
    const String& valueField,
    LedState* outStates,
    size_t count,
    ParseStats* outStats) {
  int parsedCount = 0;
  int unknownCount = 0;

  for (JsonVariantConst item : array) {
    const String location = String((const char*)(item[locationField.c_str()] | ""));
    if (location.isEmpty()) {
      ++unknownCount;
      continue;
    }

    const int ledIndex = locationRegistry_.findLedIndexByKey(location);
    if (ledIndex < 0 || static_cast<size_t>(ledIndex) >= count) {
      ++unknownCount;
      continue;
    }

    JsonVariantConst value = item[valueField.c_str()];
    if (!value.is<float>() && !value.is<int>()) {
      ++unknownCount;
      continue;
    }

    outStates[ledIndex].active = true;
    outStates[ledIndex].hasNumericValue = true;
    outStates[ledIndex].numericValue = value.as<float>();
    ++parsedCount;
  }

  if (outStats != nullptr) {
    outStats->recognizedCount = parsedCount;
    outStats->unknownCount = unknownCount;
    outStats->activeCount = parsedCount;
  }

  if (parsedCount == 0) {
    Serial.println("DataParser: NAMED_VALUE_FIELD parsed no values");
    if (outStats != nullptr) {
      outStats->error = "NAMED_VALUE_FIELD parsed no values";
    }
    return false;
  }

  return true;
}

bool DataParser::parseNamedColorField(
    JsonArrayConst array,
    const String& locationField,
    const String& colorField,
    LedState* outStates,
    size_t count,
    ParseStats* outStats) {
  int parsedCount = 0;
  int unknownCount = 0;

  for (JsonVariantConst item : array) {
    const String location = String((const char*)(item[locationField.c_str()] | ""));
    const String colorHex = String((const char*)(item[colorField.c_str()] | ""));

    if (location.isEmpty() || colorHex.isEmpty()) {
      ++unknownCount;
      continue;
    }

    const int ledIndex = locationRegistry_.findLedIndexByKey(location);
    if (ledIndex < 0 || static_cast<size_t>(ledIndex) >= count) {
      ++unknownCount;
      continue;
    }

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    if (!parseHexColor(colorHex, r, g, b)) {
      ++unknownCount;
      continue;
    }

    outStates[ledIndex].active = true;
    outStates[ledIndex].hasNumericValue = false;
    outStates[ledIndex].r = r;
    outStates[ledIndex].g = g;
    outStates[ledIndex].b = b;
    ++parsedCount;
  }

  if (outStats != nullptr) {
    outStats->recognizedCount = parsedCount;
    outStats->unknownCount = unknownCount;
    outStats->activeCount = parsedCount;
  }

  if (parsedCount == 0) {
    Serial.println("DataParser: NAMED_COLOR_FIELD parsed no values");
    if (outStats != nullptr) {
      outStats->error = "NAMED_COLOR_FIELD parsed no values";
    }
    return false;
  }

  return true;
}

bool DataParser::parseObjectListIdRgb(
    JsonObjectConst rootObject,
    LedState* outStates,
    size_t count,
    ParseStats* outStats) const {
  JsonArrayConst list = rootObject["seznam"].as<JsonArrayConst>();
  if (list.isNull()) {
    Serial.println("DataParser: OBJECT_LIST_ID_RGB missing 'seznam' array");
    if (outStats != nullptr) {
      outStats->error = "OBJECT_LIST_ID_RGB missing 'seznam' array";
    }
    return false;
  }

  int recognizedCount = 0;
  int unknownCount = 0;
  String lastItemError = "";

  for (JsonVariantConst item : list) {
    if (!item.is<JsonObjectConst>()) {
      ++unknownCount;
      lastItemError = "OBJECT_LIST_ID_RGB item is not object";
      continue;
    }

    JsonVariantConst id = item["id"];
    if (!id.is<int>()) {
      ++unknownCount;
      lastItemError = "OBJECT_LIST_ID_RGB invalid id";
      continue;
    }

    const int ledIndex = id.as<int>() - 1;
    if (ledIndex < 0 || static_cast<size_t>(ledIndex) >= count) {
      ++unknownCount;
      lastItemError = "OBJECT_LIST_ID_RGB id out of range";
      continue;
    }

    JsonVariantConst rValue = item["r"];
    JsonVariantConst gValue = item["g"];
    JsonVariantConst bValue = item["b"];
    if (!rValue.is<int>() || !gValue.is<int>() || !bValue.is<int>()) {
      ++unknownCount;
      lastItemError = "OBJECT_LIST_ID_RGB missing/invalid rgb component";
      continue;
    }

    const int r = rValue.as<int>();
    const int g = gValue.as<int>();
    const int b = bValue.as<int>();
    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
      ++unknownCount;
      lastItemError = "OBJECT_LIST_ID_RGB rgb component out of range";
      continue;
    }

    outStates[ledIndex].active = true;
    outStates[ledIndex].hasNumericValue = false;
    outStates[ledIndex].r = static_cast<uint8_t>(r);
    outStates[ledIndex].g = static_cast<uint8_t>(g);
    outStates[ledIndex].b = static_cast<uint8_t>(b);
    ++recognizedCount;
  }

  int activeCount = 0;
  for (size_t i = 0; i < count; ++i) {
    if (outStates[i].active) {
      ++activeCount;
    }
  }

  if (outStats != nullptr) {
    outStats->recognizedCount = recognizedCount;
    outStats->unknownCount = unknownCount;
    outStats->activeCount = activeCount;
    if (recognizedCount == 0 && outStats->error.isEmpty()) {
      outStats->error = "OBJECT_LIST_ID_RGB parsed no values";
    } else if (!lastItemError.isEmpty()) {
      outStats->error = lastItemError;
    }
  }

  if (recognizedCount == 0) {
    Serial.println("DataParser: OBJECT_LIST_ID_RGB parsed no values");
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

void DataParser::resetStats(ParseStats* outStats) const {
  if (outStats == nullptr) {
    return;
  }

  outStats->recognizedCount = 0;
  outStats->unknownCount = 0;
  outStats->activeCount = 0;
  outStats->error = "";
}
