#pragma once

#include <Arduino.h>

#include "AppConfig.h"
#include "AppStatus.h"
#include "ConfigStore.h"
#include "DataParser.h"
#include "HttpService.h"
#include "LedRenderer.h"
#include "WebConfigServer.h"
#include "WifiService.h"

class App {
 public:
  void begin();
  void loop();

 private:
  void printActiveConfig() const;
  AppStatus getStatus() const;
  String getConfigJson() const;
  SaveConfigResult saveConfigFromJson(const String& json);
  TestFetchResult runTestFetch();
  void getCurrentLedStates(LedState* outStates, size_t count) const;

  AppConfig config_ = AppDefaults::defaultConfig();

  ConfigStore configStore_;
  WifiService wifiService_;
  HttpService httpService_;
  DataParser dataParser_;
  LedRenderer ledRenderer_;
  WebConfigServer webConfigServer_;

  bool littleFsMounted_ = false;
  bool lastFetchOk_ = false;
  int lastHttpStatus_ = 0;
  String lastParserError_;
  int recognizedCount_ = 0;
  int unknownCount_ = 0;
  int activeCount_ = 0;

  unsigned long lastFetchMs_ = 0;
  LedState currentStates_[AppDefaults::LED_COUNT]{};
  LedState parsedStates_[AppDefaults::LED_COUNT]{};
};
