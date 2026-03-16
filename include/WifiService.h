#pragma once

#include <Arduino.h>

#include "AppConfig.h"

class WifiService {
 public:
  void begin(const WifiConfig& config);
  bool isConnected() const;
  String localIp() const;
};
