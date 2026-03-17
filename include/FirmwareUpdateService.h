#pragma once

#include <Arduino.h>
#include <WebServer.h>

// Handles OTA firmware update via HTTP multipart upload.
// Usage: register both handleUploadChunk and handleUploadFinish as the
// upload and completion callbacks on the POST /api/firmware/upload route.
class FirmwareUpdateService {
 public:
  // Called by WebServer for each incoming upload chunk.
  void handleUploadChunk(WebServer& server);

  // Called by WebServer once the HTTP POST body is fully received.
  void handleUploadFinish(WebServer& server);

 private:
  bool updateStarted_ = false;
  bool firstChunk_ = true;
  String uploadError_;
};
