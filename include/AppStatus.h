#pragma once

#include <Arduino.h>

struct AppStatus {
  bool wifiConnected;
  String ip;
  String hostname;
  unsigned long uptimeMs;
  bool lastFetchOk;
  int lastHttpStatus;
  unsigned long lastFetchMs;
  String parserType;
  String dataUrl;
  uint32_t freeHeap;
  bool littleFsMounted;
  int recognizedCount;
  int unknownCount;
  int activeCount;
  String lastParserError;
};

struct SaveConfigResult {
  bool ok;
  bool requiresWifiReconnect;
  bool requiresRestart;
  String message;
};

struct TestFetchResult {
  bool ok;
  int httpStatus;
  bool parserOk;
  String payloadPreview;
  int recognizedCount;
  int unknownCount;
  int activeCount;
  String error;
};
