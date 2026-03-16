#pragma once

#include <Arduino.h>

class MapParser {
 public:
  bool parse(const String& payload, int* outValues, size_t outCount);
};
