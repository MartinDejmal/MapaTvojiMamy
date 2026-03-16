#include "App.h"

namespace {
String maskedPassword(const String& password) {
  if (password.isEmpty()) {
    return "";
  }
  return "********";
}
}  // namespace

void App::begin() {
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println();

  if (!configStore_.begin()) {
    Serial.println("App: ConfigStore init failed, using defaults only");
    config_ = AppDefaults::defaultConfig();
  } else if (!configStore_.load(config_)) {
    config_ = AppDefaults::defaultConfig();
    configStore_.save(config_);
  }

  printActiveConfig();

  wifiService_.begin(config_.wifi);
  ledRenderer_.begin(config_.render);
}

void App::loop() {
  const unsigned long now = millis();
  if ((now - lastFetchMs_) < config_.mapProfile.refreshIntervalMs) {
    return;
  }

  lastFetchMs_ = now;

  if (!wifiService_.isConnected()) {
    Serial.println("WiFi disconnected");
    return;
  }

  String payload;
  if (!httpService_.get(config_.mapProfile.url, payload)) {
    return;
  }

  if (!dataParser_.parse(payload, config_, parsedStates_, AppDefaults::LED_COUNT)) {
    Serial.println("App: parser failed, keeping previous LED state");
    return;
  }

  memcpy(currentStates_, parsedStates_, sizeof(currentStates_));
  ledRenderer_.render(currentStates_, AppDefaults::LED_COUNT, config_.mapProfile);
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
