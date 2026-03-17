#include "FirmwareUpdateService.h"

#include <ArduinoJson.h>
#include <Update.h>

// First byte of a valid ESP32 firmware image (ROM bootloader magic).
static constexpr uint8_t kEspImageMagic = 0xE9;

void FirmwareUpdateService::handleUploadChunk(WebServer& server) {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    updateStarted_ = false;
    firstChunk_ = true;
    uploadError_ = "";

    // Validate file extension.
    if (!upload.filename.endsWith(".bin")) {
      uploadError_ = "Soubor nemá příponu .bin";
      Serial.println("FirmwareUpdate: rejected – bad extension: " + upload.filename);
      return;
    }

    // Reserve OTA partition.  UPDATE_SIZE_UNKNOWN lets the library use the
    // entire available OTA slot, which is the safe default.
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      uploadError_ = String("Nelze zahájit OTA update: ") + Update.errorString();
      Serial.println("FirmwareUpdate: Update.begin failed: " + uploadError_);
      return;
    }

    updateStarted_ = true;
    Serial.println("FirmwareUpdate: upload started – " + upload.filename);

  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!updateStarted_) {
      // A previous step already failed; discard incoming data silently.
      return;
    }

    // Validate magic byte on the first chunk that actually contains data.
    // Empty chunks can occur due to network chunking; defer until data arrives.
    if (firstChunk_ && upload.bufLen > 0) {
      firstChunk_ = false;
      if (upload.buf[0] != kEspImageMagic) {
        uploadError_ = "Neplatný firmware image (nesprávný magic byte)";
        Serial.println("FirmwareUpdate: " + uploadError_);
        Update.abort();
        updateStarted_ = false;
        return;
      }
    }

    if (upload.bufLen > 0 &&
        Update.write(upload.buf, upload.bufLen) != upload.bufLen) {
      uploadError_ = String("Chyba zápisu firmware: ") + Update.errorString();
      Serial.println("FirmwareUpdate: " + uploadError_);
      Update.abort();
      updateStarted_ = false;
    }

  } else if (upload.status == UPLOAD_FILE_END) {
    // upload.totalSize accumulates during WRITE events; 0 means no data arrived.
    if (updateStarted_ && upload.totalSize == 0) {
      uploadError_ = "Prázdný soubor";
      Update.abort();
      updateStarted_ = false;
    }

  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (updateStarted_) {
      Update.abort();
      updateStarted_ = false;
    }
    uploadError_ = "Upload přerušen klientem";
  }
}

void FirmwareUpdateService::handleUploadFinish(WebServer& server) {
  StaticJsonDocument<256> doc;

  if (!uploadError_.isEmpty()) {
    doc["ok"] = false;
    doc["error"] = uploadError_;
    String response;
    serializeJson(doc, response);
    server.send(400, "application/json", response);
    return;
  }

  if (!updateStarted_) {
    doc["ok"] = false;
    doc["error"] = "Upload nebyl zahájen nebo byl odmítnut";
    String response;
    serializeJson(doc, response);
    server.send(400, "application/json", response);
    return;
  }

  if (!Update.end(true)) {
    doc["ok"] = false;
    doc["error"] = String("OTA update selhal: ") + Update.errorString();
    String response;
    serializeJson(doc, response);
    server.send(500, "application/json", response);
    return;
  }

  Serial.println("FirmwareUpdate: update OK, restarting...");
  doc["ok"] = true;
  doc["message"] = "Firmware nahrán. Zařízení se restartuje.";
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);

  // Give the HTTP response time to reach the client before rebooting.
  delay(500);
  ESP.restart();
}
