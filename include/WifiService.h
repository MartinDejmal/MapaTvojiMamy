#pragma once

#include <Arduino.h>

class WifiService {
 public:
  void begin();
  bool isConnected() const;
  String localIp() const;
};
