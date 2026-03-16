#include "WebConfigServer.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

void WebConfigServer::begin(
    StatusProvider statusProvider,
    ConfigJsonProvider configJsonProvider,
    ConfigSaver configSaver,
    TestFetchRunner testFetchRunner) {
  statusProvider_ = statusProvider;
  configJsonProvider_ = configJsonProvider;
  configSaver_ = configSaver;
  testFetchRunner_ = testFetchRunner;

  registerRoutes();
  server_.begin();
  Serial.println("WebConfigServer: started on port 80");
}

void WebConfigServer::handleClient() {
  server_.handleClient();
}

void WebConfigServer::registerRoutes() {
  server_.on("/", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/app.js", HTTP_GET, [this]() { handleAppJs(); });
  server_.on("/app.css", HTTP_GET, [this]() { handleAppCss(); });

  server_.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
  server_.on("/api/config", HTTP_GET, [this]() { handleGetConfig(); });
  server_.on("/api/config", HTTP_POST, [this]() { handlePostConfig(); });
  server_.on("/api/test-fetch", HTTP_POST, [this]() { handleTestFetch(); });
  server_.on("/api/restart", HTTP_POST, [this]() { handleRestart(); });

  server_.onNotFound([this]() {
    if (serveStaticFile(server_.uri().c_str(), "text/plain")) {
      return;
    }
    server_.send(404, "application/json", "{\"ok\":false,\"error\":\"not found\"}");
  });
}

void WebConfigServer::handleStatus() {
  if (!statusProvider_) {
    server_.send(500, "application/json", "{\"ok\":false,\"error\":\"status callback missing\"}");
    return;
  }

  const AppStatus status = statusProvider_();
  StaticJsonDocument<1024> doc;
  doc["wifiConnected"] = status.wifiConnected;
  doc["ip"] = status.ip;
  doc["hostname"] = status.hostname;
  doc["uptimeMs"] = status.uptimeMs;
  doc["lastFetchOk"] = status.lastFetchOk;
  doc["lastHttpStatus"] = status.lastHttpStatus;
  doc["lastFetchMs"] = status.lastFetchMs;
  doc["parserType"] = status.parserType;
  doc["dataUrl"] = status.dataUrl;
  doc["freeHeap"] = status.freeHeap;
  doc["littleFsMounted"] = status.littleFsMounted;
  doc["recognizedCount"] = status.recognizedCount;
  doc["unknownCount"] = status.unknownCount;
  doc["activeCount"] = status.activeCount;
  doc["lastParserError"] = status.lastParserError;

  String body;
  serializeJson(doc, body);
  server_.send(200, "application/json", body);
}

void WebConfigServer::handleGetConfig() {
  if (!configJsonProvider_) {
    server_.send(500, "application/json", "{\"ok\":false,\"error\":\"config callback missing\"}");
    return;
  }

  server_.send(200, "application/json", configJsonProvider_());
}

void WebConfigServer::handlePostConfig() {
  if (!configSaver_) {
    server_.send(500, "application/json", "{\"ok\":false,\"error\":\"save callback missing\"}");
    return;
  }

  const String body = server_.arg("plain");
  SaveConfigResult result = configSaver_(body);

  StaticJsonDocument<512> doc;
  doc["ok"] = result.ok;
  doc["requiresWifiReconnect"] = result.requiresWifiReconnect;
  doc["requiresRestart"] = result.requiresRestart;
  doc["message"] = result.message;

  String response;
  serializeJson(doc, response);
  server_.send(result.ok ? 200 : 400, "application/json", response);
}

void WebConfigServer::handleTestFetch() {
  if (!testFetchRunner_) {
    server_.send(500, "application/json", "{\"ok\":false,\"error\":\"test fetch callback missing\"}");
    return;
  }

  const TestFetchResult result = testFetchRunner_();
  StaticJsonDocument<2048> doc;
  doc["ok"] = result.ok;
  doc["httpStatus"] = result.httpStatus;
  doc["parserOk"] = result.parserOk;
  doc["payloadPreview"] = result.payloadPreview;
  doc["recognizedCount"] = result.recognizedCount;
  doc["unknownCount"] = result.unknownCount;
  doc["activeCount"] = result.activeCount;
  doc["error"] = result.error;

  String response;
  serializeJson(doc, response);
  server_.send(result.ok ? 200 : 500, "application/json", response);
}

void WebConfigServer::handleRestart() {
  server_.send(200, "application/json", "{\"ok\":true,\"message\":\"Restarting\"}");
  delay(300);
  ESP.restart();
}

void WebConfigServer::handleRoot() {
  if (serveStaticFile("/index.html", "text/html")) {
    return;
  }
  server_.send(404, "text/plain", "index.html not found in LittleFS");
}

void WebConfigServer::handleAppJs() {
  if (serveStaticFile("/app.js", "application/javascript")) {
    return;
  }
  server_.send(404, "text/plain", "app.js not found in LittleFS");
}

void WebConfigServer::handleAppCss() {
  if (serveStaticFile("/app.css", "text/css")) {
    return;
  }
  server_.send(404, "text/plain", "app.css not found in LittleFS");
}

bool WebConfigServer::serveStaticFile(const char* path, const char* contentType) {
  if (!LittleFS.exists(path)) {
    return false;
  }

  File file = LittleFS.open(path, "r");
  if (!file) {
    return false;
  }

  server_.streamFile(file, contentType);
  file.close();
  return true;
}
