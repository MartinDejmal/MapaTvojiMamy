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

struct ParseStats {
  int recognizedCount;
  int unknownCount;
  int activeCount;
  String error;
};

class DataParser {
 public:
  bool parse(
      const String& payload,
      const AppConfig& config,
      LedState* outStates,
      size_t count,
      ParseStats* outStats = nullptr);

 private:
  bool parseIndexedH1(
      JsonArrayConst array,
      LedState* outStates,
      size_t count,
      ParseStats* outStats) const;
  bool parseIndexedValueField(
      JsonArrayConst array,
      const String& valueField,
      LedState* outStates,
      size_t count,
      ParseStats* outStats) const;
  bool parseNamedValueField(
      JsonArrayConst array,
      const String& locationField,
      const String& valueField,
      LedState* outStates,
      size_t count,
      ParseStats* outStats);
  bool parseNamedColorField(
      JsonArrayConst array,
      const String& locationField,
      const String& colorField,
      LedState* outStates,
      size_t count,
      ParseStats* outStats);
  bool parseHexColor(const String& hex, uint8_t& r, uint8_t& g, uint8_t& b) const;
  void clearStates(LedState* outStates, size_t count) const;
  void resetStats(ParseStats* outStats) const;

  LocationRegistry locationRegistry_;
};
