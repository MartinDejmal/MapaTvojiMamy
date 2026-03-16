#include "App.h"

#include "AppConfig.h"

void App::begin() {
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println();

  wifiService_.begin();
  ledRenderer_.begin();
}

void App::loop() {
  const unsigned long now = millis();
  if ((now - lastFetchMs_) < AppConfig::FETCH_INTERVAL_MS) {
    return;
  }

  lastFetchMs_ = now;

  if (!wifiService_.isConnected()) {
    Serial.println("WiFi Disconnected");
    return;
  }

  String payload;
  if (!httpService_.get(AppConfig::JSON_URL, payload)) {
    return;
  }

  if (!mapParser_.parse(payload, temperatureValues_, AppConfig::LED_COUNT)) {
    return;
  }

  ledRenderer_.renderTemperatures(temperatureValues_, AppConfig::LED_COUNT);
}
