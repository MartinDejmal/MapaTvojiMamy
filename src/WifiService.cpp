#include "WifiService.h"

#include <WiFi.h>

#include "AppConfig.h"

void WifiService::begin() {
  Serial.print("Connecting to ");
  Serial.println(AppConfig::WIFI_SSID);

  WiFi.begin(AppConfig::WIFI_SSID, AppConfig::WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(localIp());
}

bool WifiService::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

String WifiService::localIp() const {
  return WiFi.localIP().toString();
}
