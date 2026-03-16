#pragma once

#include <Arduino.h>

#include "AppConfig.h"

class ConfigStore {
 public:
  bool begin();
  bool load(AppConfig& outConfig);
  bool save(const AppConfig& config);
  bool validateAndNormalize(AppConfig& config, String& reason);
  bool fromJson(const String& json, AppConfig& outConfig, String& reason);
  String toJson(const AppConfig& config) const;
};
