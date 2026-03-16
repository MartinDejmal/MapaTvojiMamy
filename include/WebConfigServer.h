#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include <functional>

#include "AppStatus.h"

class WebConfigServer {
 public:
  using StatusProvider = std::function<AppStatus()>;
  using ConfigJsonProvider = std::function<String()>;
  using ConfigSaver = std::function<SaveConfigResult(const String&)>;
  using TestFetchRunner = std::function<TestFetchResult()>;

  void begin(
      StatusProvider statusProvider,
      ConfigJsonProvider configJsonProvider,
      ConfigSaver configSaver,
      TestFetchRunner testFetchRunner);
  void handleClient();

 private:
  void registerRoutes();
  void handleStatus();
  void handleGetConfig();
  void handlePostConfig();
  void handleTestFetch();
  void handleRestart();
  void handleRoot();
  void handleAppJs();
  void handleAppCss();
  bool serveStaticFile(const char* path, const char* contentType);

  WebServer server_{80};
  StatusProvider statusProvider_;
  ConfigJsonProvider configJsonProvider_;
  ConfigSaver configSaver_;
  TestFetchRunner testFetchRunner_;
};
