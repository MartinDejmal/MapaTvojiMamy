#pragma once

#include <Arduino.h>

#include "AppConfig.h"
#include "HttpService.h"
#include "LedRenderer.h"
#include "MapParser.h"
#include "WifiService.h"

class App {
 public:
  void begin();
  void loop();

 private:
  WifiService wifiService_;
  HttpService httpService_;
  MapParser mapParser_;
  LedRenderer ledRenderer_;

  unsigned long lastFetchMs_ = 0;
  int temperatureValues_[AppConfig::LED_COUNT] = {0};
};
