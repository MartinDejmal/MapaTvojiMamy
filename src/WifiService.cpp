#include "WifiService.h"

#include <WiFi.h>

void WifiService::begin(const WifiConfig& config) {
  if (!config.ssid.isEmpty()) {
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
  } else {
    Serial.println("No WiFi SSID configured");
  }

  // Fallback: start AP mode for WiFi provisioning
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);

  apSsid_ = config.hostname.isEmpty() ? "cr-mapa-setup" : (config.hostname + "-setup");

  if (WiFi.softAP(apSsid_.c_str())) {
    apMode_ = true;
    // Redirect all DNS queries to the AP IP for captive portal
    dnsServer_.start(53, "*", WiFi.softAPIP());
    Serial.println("AP mode started for WiFi provisioning");
    Serial.println("DNS server started for captive portal redirect");
    Serial.print("AP SSID: ");
    Serial.println(apSsid_);
    Serial.print("AP IP: ");
    Serial.println(apIp());
  } else {
    Serial.println("AP mode failed to start");
  }
}

void WifiService::processDns() {
  if (apMode_) {
    dnsServer_.processNextRequest();
  }
}

bool WifiService::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

bool WifiService::isApMode() const {
  return apMode_;
}

String WifiService::localIp() const {
  return WiFi.localIP().toString();
}

String WifiService::apSsid() const {
  return apSsid_;
}

String WifiService::apIp() const {
  return WiFi.softAPIP().toString();
}
