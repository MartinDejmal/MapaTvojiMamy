#pragma once

#include <Arduino.h>

#include "AppConfig.h"

class WifiService {
 public:
  void begin(const WifiConfig& config);
  bool isConnected() const;
  bool isApMode() const;
  String localIp() const;
  String apSsid() const;
  String apIp() const;

 private:
  bool apMode_ = false;
  String apSsid_;
};
