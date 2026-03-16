#pragma once

#include <Arduino.h>

#include "AppConfig.h"
#include "ConfigStore.h"
#include "DataParser.h"
#include "HttpService.h"
#include "LedRenderer.h"
#include "WifiService.h"

class App {
 public:
  void begin();
  void loop();

 private:
  void printActiveConfig() const;

  AppConfig config_ = AppDefaults::defaultConfig();

  ConfigStore configStore_;
  WifiService wifiService_;
  HttpService httpService_;
  DataParser dataParser_;
  LedRenderer ledRenderer_;

  unsigned long lastFetchMs_ = 0;
  LedState currentStates_[AppDefaults::LED_COUNT]{};
  LedState parsedStates_[AppDefaults::LED_COUNT]{};
};
