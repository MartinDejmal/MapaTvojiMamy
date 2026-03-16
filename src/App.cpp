#include "App.h"

#include <ArduinoJson.h>

namespace {
String maskedPassword(const String& password) {
  if (password.isEmpty()) {
    return "";
  }
  return "********";
}

String previewPayload(const String& payload) {
  constexpr size_t kMaxPreviewLength = 240;
  if (payload.length() <= kMaxPreviewLength) {
    return payload;
  }
  return payload.substring(0, kMaxPreviewLength) + "...";
}
}  // namespace

void App::begin() {
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println();

  littleFsMounted_ = configStore_.begin();
  if (!littleFsMounted_) {
    Serial.println("App: ConfigStore init failed, using defaults only");
    config_ = AppDefaults::defaultConfig();
  } else if (!configStore_.load(config_)) {
    config_ = AppDefaults::defaultConfig();
    configStore_.save(config_);
  }

  printActiveConfig();

  wifiService_.begin(config_.wifi);
  ledRenderer_.begin(config_.render);

  webConfigServer_.begin(
      [this]() { return getStatus(); },
      [this]() { return getConfigJson(); },
      [this](const String& json) { return saveConfigFromJson(json); },
      [this]() { return runTestFetch(); });
}

void App::loop() {
  webConfigServer_.handleClient();

  const unsigned long now = millis();
  if ((now - lastFetchMs_) < config_.mapProfile.refreshIntervalMs) {
    return;
  }

  lastFetchMs_ = now;

  if (!wifiService_.isConnected()) {
    Serial.println("WiFi disconnected");
    lastFetchOk_ = false;
    lastHttpStatus_ = 0;
    return;
  }

  String payload;
  HttpResult httpResult = httpService_.getWithStatus(config_.mapProfile.url, payload);
  lastHttpStatus_ = httpResult.httpStatus;
  if (!httpResult.ok) {
    lastFetchOk_ = false;
    return;
  }

  ParseStats parseStats;
  if (!dataParser_.parse(payload, config_, parsedStates_, AppDefaults::LED_COUNT, &parseStats)) {
    Serial.println("App: parser failed, keeping previous LED state");
    lastFetchOk_ = false;
    lastParserError_ = parseStats.error;
    recognizedCount_ = parseStats.recognizedCount;
    unknownCount_ = parseStats.unknownCount;
    activeCount_ = parseStats.activeCount;
    return;
  }

  memcpy(currentStates_, parsedStates_, sizeof(currentStates_));
  ledRenderer_.render(currentStates_, AppDefaults::LED_COUNT, config_.mapProfile);

  lastFetchOk_ = true;
  lastParserError_ = "";
  recognizedCount_ = parseStats.recognizedCount;
  unknownCount_ = parseStats.unknownCount;
  activeCount_ = parseStats.activeCount;
}

AppStatus App::getStatus() const {
  AppStatus status;
  status.wifiConnected = wifiService_.isConnected();
  status.ip = wifiService_.localIp();
  status.hostname = config_.wifi.hostname;
  status.uptimeMs = millis();
  status.lastFetchOk = lastFetchOk_;
  status.lastHttpStatus = lastHttpStatus_;
  status.lastFetchMs = lastFetchMs_;
  status.parserType = config_.mapProfile.parserType;
  status.dataUrl = config_.mapProfile.url;
  status.freeHeap = ESP.getFreeHeap();
  status.littleFsMounted = littleFsMounted_;
  status.recognizedCount = recognizedCount_;
  status.unknownCount = unknownCount_;
  status.activeCount = activeCount_;
  status.lastParserError = lastParserError_;
  return status;
}

String App::getConfigJson() const {
  return configStore_.toJson(config_);
}

SaveConfigResult App::saveConfigFromJson(const String& json) {
  SaveConfigResult result{false, false, false, ""};

  AppConfig candidate;
  String reason;
  if (!configStore_.fromJson(json, candidate, reason)) {
    result.message = reason;
    return result;
  }

  const bool wifiChanged = candidate.wifi.ssid != config_.wifi.ssid ||
                           candidate.wifi.password != config_.wifi.password ||
                           candidate.wifi.hostname != config_.wifi.hostname;

  if (!configStore_.save(candidate)) {
    result.message = "Failed to save config";
    return result;
  }

  config_ = candidate;
  ledRenderer_.begin(config_.render);

  result.ok = true;
  result.requiresWifiReconnect = wifiChanged;
  result.requiresRestart = wifiChanged;
  result.message = wifiChanged ? "Saved. Wi-Fi changes require reconnect/restart." : "Saved and applied.";
  return result;
}

TestFetchResult App::runTestFetch() {
  TestFetchResult result{false, 0, false, "", 0, 0, 0, ""};

  if (!wifiService_.isConnected()) {
    result.error = "Wi-Fi is not connected";
    return result;
  }

  String payload;
  HttpResult httpResult = httpService_.getWithStatus(config_.mapProfile.url, payload);
  result.httpStatus = httpResult.httpStatus;
  result.payloadPreview = previewPayload(payload);
  if (!httpResult.ok) {
    result.error = httpResult.error;
    return result;
  }

  LedState tempStates[AppDefaults::LED_COUNT]{};
  ParseStats parseStats;
  const bool parseOk = dataParser_.parse(payload, config_, tempStates, AppDefaults::LED_COUNT, &parseStats);
  result.parserOk = parseOk;
  result.recognizedCount = parseStats.recognizedCount;
  result.unknownCount = parseStats.unknownCount;
  result.activeCount = parseStats.activeCount;
  result.error = parseOk ? "" : parseStats.error;
  result.ok = parseOk;

  return result;
}

void App::printActiveConfig() const {
  Serial.println("Active config:");
  Serial.print("  schemaVersion: ");
  Serial.println(config_.schemaVersion);

  Serial.println("  wifi:");
  Serial.print("    ssid: ");
  Serial.println(config_.wifi.ssid);
  Serial.print("    password: ");
  Serial.println(maskedPassword(config_.wifi.password));
  Serial.print("    hostname: ");
  Serial.println(config_.wifi.hostname);

  Serial.println("  mapProfile:");
  Serial.print("    url: ");
  Serial.println(config_.mapProfile.url);
  Serial.print("    parserType: ");
  Serial.println(config_.mapProfile.parserType);
  Serial.print("    locationField: ");
  Serial.println(config_.mapProfile.locationField);
  Serial.print("    valueField: ");
  Serial.println(config_.mapProfile.valueField);
  Serial.print("    colorField: ");
  Serial.println(config_.mapProfile.colorField);
  Serial.print("    minValue: ");
  Serial.println(config_.mapProfile.minValue);
  Serial.print("    maxValue: ");
  Serial.println(config_.mapProfile.maxValue);
  Serial.print("    refreshIntervalMs: ");
  Serial.println(config_.mapProfile.refreshIntervalMs);

  Serial.println("  render:");
  Serial.print("    brightness: ");
  Serial.println(config_.render.brightness);
  Serial.print("    wheelMin: ");
  Serial.println(config_.render.wheelMin);
  Serial.print("    wheelMax: ");
  Serial.println(config_.render.wheelMax);
}
