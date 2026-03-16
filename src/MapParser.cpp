#include "MapParser.h"

#include <ArduinoJson.h>

bool MapParser::parse(const String& payload, int* outValues, size_t outCount) {
  // Pole 77 objektů s jednou numerickou hodnotou "h1" + rezerva.
  // Cíleno na současný payload, aby zůstal memory footprint rozumný.
  StaticJsonDocument<4096> doc;
  const DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("JSON parse failed: ");
    Serial.println(error.c_str());
    return false;
  }

  if (!doc.is<JsonArray>()) {
    Serial.println("JSON parse failed: root is not array");
    return false;
  }

  JsonArray arr = doc.as<JsonArray>();
  if (arr.size() < outCount) {
    Serial.println("JSON parse failed: array too short");
    return false;
  }

  for (size_t i = 0; i < outCount; ++i) {
    JsonVariant h1 = arr[i]["h1"];
    if (h1.isNull()) {
      Serial.print("JSON parse failed: missing h1 at index ");
      Serial.println(i);
      return false;
    }
    outValues[i] = h1.as<int>();
  }

  return true;
}
