#include "WebConfigServer.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

void WebConfigServer::begin(
    bool apMode,
    const String& apIp,
    StatusProvider statusProvider,
    ConfigJsonProvider configJsonProvider,
    ConfigSaver configSaver,
    TestFetchRunner testFetchRunner,
    LedStatesProvider ledStatesProvider) {
  apMode_ = apMode;
  apIp_ = apIp;
  statusProvider_ = statusProvider;
  configJsonProvider_ = configJsonProvider;
  configSaver_ = configSaver;
  testFetchRunner_ = testFetchRunner;
  ledStatesProvider_ = ledStatesProvider;

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
  server_.on("/api/map-state", HTTP_GET, [this]() { handleMapState(); });

  server_.on("/api/firmware/upload", HTTP_POST,
             [this]() { handleFirmwareUploadFinish(); },
             [this]() { handleFirmwareUploadChunk(); });

  // Captive portal detection endpoints for iOS, Android, Windows
  if (apMode_) {
    // iOS / macOS
    server_.on("/hotspot-detect.html", HTTP_GET, [this]() { handleCaptivePortalRedirect(); });
    // Android
    server_.on("/generate_204", HTTP_GET, [this]() { handleCaptivePortalRedirect(); });
    // Windows 10+
    server_.on("/connecttest.txt", HTTP_GET, [this]() { handleCaptivePortalRedirect(); });
    // Windows NCSI
    server_.on("/ncsi.txt", HTTP_GET, [this]() { handleCaptivePortalRedirect(); });
  }

  server_.onNotFound([this]() {
    if (apMode_) {
      handleCaptivePortalRedirect();
      return;
    }
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
  // 1536 bytes accommodates all fields including the new firmware metadata strings.
  StaticJsonDocument<1536> doc;
  doc["wifiConnected"] = status.wifiConnected;
  doc["apMode"] = status.apMode;
  doc["apSsid"] = status.apSsid;
  doc["apIp"] = status.apIp;
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
  doc["firmwareVersion"] = status.firmwareVersion;
  doc["buildDate"] = status.buildDate;
  doc["buildTime"] = status.buildTime;
  doc["otaSupported"] = status.otaSupported;

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

void WebConfigServer::handleMapState() {
  if (!ledStatesProvider_) {
    server_.send(500, "application/json", "{\"ok\":false,\"error\":\"map callback missing\"}");
    return;
  }

  constexpr size_t kLedCount = AppDefaults::LED_COUNT;
  LedState states[kLedCount]{};
  ledStatesProvider_(states, kLedCount);

  DynamicJsonDocument doc(24576);
  JsonObject image = doc.createNestedObject("image");
  image["url"] = "/mapa-okresy-cr.jpg";
  image["width"] = MapLayout::MAP_IMAGE_WIDTH;
  image["height"] = MapLayout::MAP_IMAGE_HEIGHT;

  JsonArray points = doc.createNestedArray("points");
  const MapPoint* mapPoints = mapLayout_.points();
  const size_t pointCount = mapLayout_.count();
  for (size_t i = 0; i < pointCount; ++i) {
    const MapPoint& point = mapPoints[i];
    const bool indexValid = point.index < kLedCount;
    const LedState state = indexValid ? states[point.index] : LedState{};

    JsonObject item = points.createNestedObject();
    item["index"] = point.index;
    item["name"] = point.name;
    item["x"] = point.x;
    item["y"] = point.y;
    item["active"] = state.active;
    item["hasNumericValue"] = state.hasNumericValue;
    item["numericValue"] = state.numericValue;
    item["r"] = state.r;
    item["g"] = state.g;
    item["b"] = state.b;
  }

  String response;
  serializeJson(doc, response);
  server_.send(200, "application/json", response);
}

void WebConfigServer::handleRestart() {
  server_.send(200, "application/json", "{\"ok\":true,\"message\":\"Restarting\"}");
  delay(300);
  ESP.restart();
}

void WebConfigServer::handleCaptivePortalRedirect() {
  // Use the explicit IP URL: DNS has already redirected the browser from
  // a foreign domain to us, so a relative path would resolve to that foreign
  // domain and not our AP IP.
  String url = String("http://") + apIp_ + "/";
  server_.sendHeader("Location", url);
  server_.send(302, "text/plain", "");
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

void WebConfigServer::handleFirmwareUploadChunk() {
  firmwareUpdateService_.handleUploadChunk(server_);
}

void WebConfigServer::handleFirmwareUploadFinish() {
  firmwareUpdateService_.handleUploadFinish(server_);
}
