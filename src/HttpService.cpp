#include "HttpService.h"

#include <HTTPClient.h>
#include <WiFi.h>

bool HttpService::get(const String& url, String& payload) {
  WiFiClient client;
  HTTPClient http;

  if (!http.begin(client, url)) {
    Serial.println("HTTP GET failed: begin() returned false");
    return false;
  }

  const int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
    http.end();
    return true;
  }

  Serial.print("HTTP GET failed, code: ");
  Serial.println(httpResponseCode);
  http.end();
  return false;
}
