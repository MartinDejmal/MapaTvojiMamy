#pragma once

#include <Arduino.h>

struct MapPoint {
  uint8_t index;
  const char* name;
  int x;
  int y;
};

class MapLayout {
 public:
  static constexpr int MAP_IMAGE_WIDTH = 1664;
  static constexpr int MAP_IMAGE_HEIGHT = 925;

  const MapPoint* points() const;
  size_t count() const;
};
