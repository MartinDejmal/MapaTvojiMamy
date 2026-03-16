#include "WifiService.h"

#include <WiFi.h>

void WifiService::begin(const WifiConfig& config) {
  if (!config.hostname.isEmpty()) {
    WiFi.setHostname(config.hostname.c_str());
  }

  Serial.print("Connecting to ");
  Serial.println(config.ssid);

  WiFi.begin(config.ssid.c_str(), config.password.c_str());

  uint16_t retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 120) {
    delay(500);
    Serial.print(".");
    ++retries;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(localIp());
    return;
  }

  Serial.println("WiFi connect timeout");
}

bool WifiService::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

String WifiService::localIp() const {
  return WiFi.localIP().toString();
}
