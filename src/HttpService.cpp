#include "HttpService.h"

#include <HTTPClient.h>
#include <WiFi.h>

bool HttpService::get(const String& url, String& payload) {
  HttpResult result = getWithStatus(url, payload);
  return result.ok;
}

HttpResult HttpService::getWithStatus(const String& url, String& payload) {
  WiFiClient client;
  HTTPClient http;

  if (!http.begin(client, url)) {
    Serial.println("HTTP GET failed: begin() returned false");
    return {false, 0, "http.begin failed"};
  }

  http.addHeader("Accept-Encoding", "identity");
  const int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    payload = http.getString();
    http.end();
    return {true, httpResponseCode, ""};
  }

  Serial.print("HTTP GET failed, code: ");
  Serial.println(httpResponseCode);
  http.end();
  return {false, httpResponseCode, "HTTP GET failed"};
}
