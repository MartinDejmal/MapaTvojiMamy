#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include <functional>

#include "AppConfig.h"
#include "AppStatus.h"
#include "DataParser.h"
#include "FirmwareUpdateService.h"
#include "MapLayout.h"

class WebConfigServer {
 public:
  using StatusProvider = std::function<AppStatus()>;
  using ConfigJsonProvider = std::function<String()>;
  using ConfigSaver = std::function<SaveConfigResult(const String&)>;
  using TestFetchRunner = std::function<TestFetchResult()>;
  using LedStatesProvider = std::function<void(LedState* outStates, size_t count)>;

  void begin(
      bool apMode,
      const String& apIp,
      StatusProvider statusProvider,
      ConfigJsonProvider configJsonProvider,
      ConfigSaver configSaver,
      TestFetchRunner testFetchRunner,
      LedStatesProvider ledStatesProvider);
  void handleClient();

 private:
  void registerRoutes();
  void handleStatus();
  void handleGetConfig();
  void handlePostConfig();
  void handleTestFetch();
  void handleRestart();
  void handleMapState();
  void handleFirmwareUploadChunk();
  void handleFirmwareUploadFinish();
  void handleRoot();
  void handleAppJs();
  void handleAppCss();
  void handleCaptivePortalRedirect();
  bool serveStaticFile(const char* path, const char* contentType);

  WebServer server_{80};
  bool apMode_ = false;
  String apIp_;
  StatusProvider statusProvider_;
  ConfigJsonProvider configJsonProvider_;
  ConfigSaver configSaver_;
  TestFetchRunner testFetchRunner_;
  LedStatesProvider ledStatesProvider_;
  MapLayout mapLayout_;
  FirmwareUpdateService firmwareUpdateService_;
};
