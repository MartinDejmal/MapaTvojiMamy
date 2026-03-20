#include "HttpService.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

bool HttpService::get(const String& url, String& payload) {
  HttpResult result = getWithStatus(url, payload);
  return result.ok;
}

namespace {
// Performs an HTTP GET request using the provided client and returns the
// result. Adds an Accept-Encoding: identity header to prevent compressed
// responses that cannot be parsed as JSON.
HttpResult doGet(WiFiClient& client, const String& url, String& payload) {
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
}  // namespace

HttpResult HttpService::getWithStatus(const String& url, String& payload) {
  if (url.startsWith("https://")) {
    // Note: certificate verification is skipped (setInsecure) because the
    // device has no system trust store and the fetched data is public sensor
    // information. Certificate pinning or a trust store would be preferable
    // for sensitive data, but is out of scope for this project.
    WiFiClientSecure secureClient;
    secureClient.setInsecure();
    return doGet(secureClient, url, payload);
  }

  WiFiClient plainClient;
  return doGet(plainClient, url, payload);
}
