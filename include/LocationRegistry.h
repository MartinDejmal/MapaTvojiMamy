#pragma once

#include <Arduino.h>

class LocationRegistry {
 public:
  int findLedIndexByKey(const String& key) const;

 private:
  String normalizeKey(const String& key) const;
};
