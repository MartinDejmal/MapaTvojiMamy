#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include "AppConfig.h"
#include "LocationRegistry.h"

struct LedState {
  bool active;
  bool hasNumericValue;
  float numericValue;
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

class DataParser {
 public:
  bool parse(const String& payload, const AppConfig& config, LedState* outStates, size_t count);

 private:
  bool parseIndexedH1(JsonArrayConst array, LedState* outStates, size_t count) const;
  bool parseIndexedValueField(
      JsonArrayConst array,
      const String& valueField,
      LedState* outStates,
      size_t count) const;
  bool parseNamedValueField(
      JsonArrayConst array,
      const String& locationField,
      const String& valueField,
      LedState* outStates,
      size_t count);
  bool parseNamedColorField(
      JsonArrayConst array,
      const String& locationField,
      const String& colorField,
      LedState* outStates,
      size_t count);
  bool parseHexColor(const String& hex, uint8_t& r, uint8_t& g, uint8_t& b) const;
  void clearStates(LedState* outStates, size_t count) const;

  LocationRegistry locationRegistry_;
};
