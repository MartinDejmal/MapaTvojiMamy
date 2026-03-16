#include "ConfigStore.h"

#include <ArduinoJson.h>
#include <LITTLEFS.h>

namespace {
constexpr const char* kConfigPath = "/config.json";

String maskedPassword(const String& value) {
  if (value.isEmpty()) {
    return "";
  }
  return "********";
}
}  // namespace

bool ConfigStore::begin() {
  if (!LITTLEFS.begin(true)) {
    Serial.println("ConfigStore: LittleFS mount failed");
    return false;
  }

  Serial.println("ConfigStore: LittleFS mounted");
  return true;
}

bool ConfigStore::load(AppConfig& outConfig) {
  AppConfig config = AppDefaults::defaultConfig();

  if (!LITTLEFS.exists(kConfigPath)) {
    Serial.println("ConfigStore: /config.json missing, using defaults");
    outConfig = config;
    return false;
  }

  File file = LITTLEFS.open(kConfigPath, "r");
  if (!file) {
    Serial.println("ConfigStore: cannot open /config.json, using defaults");
    outConfig = config;
    return false;
  }

  const String json = file.readString();
  file.close();

  String reason;
  if (!fromJson(json, config, reason)) {
    Serial.print("ConfigStore: invalid config, fallback to defaults: ");
    Serial.println(reason);
    outConfig = AppDefaults::defaultConfig();
    return false;
  }

  outConfig = config;
  Serial.println("ConfigStore: config loaded from LittleFS");
  return true;
}

bool ConfigStore::save(const AppConfig& config) {
  File file = LITTLEFS.open(kConfigPath, "w");
  if (!file) {
    Serial.println("ConfigStore: cannot open /config.json for write");
    return false;
  }

  const String json = toJson(config);
  const size_t written = file.print(json);
  file.close();

  if (written == 0) {
    Serial.println("ConfigStore: write failed");
    return false;
  }

  Serial.println("ConfigStore: config saved to LittleFS");
  return true;
}

bool ConfigStore::validateAndNormalize(AppConfig& config, String& reason) {
  AppConfig defaults = AppDefaults::defaultConfig();

  if (config.schemaVersion != AppDefaults::SCHEMA_VERSION) {
    reason = "unsupported schemaVersion";
    return false;
  }

  if (config.mapProfile.url.isEmpty()) {
    reason = "dataSource.url empty";
    return false;
  }

  if (config.mapProfile.refreshIntervalMs < 100) {
    reason = "refreshIntervalMs too low";
    return false;
  }

  if (config.mapProfile.maxValue <= config.mapProfile.minValue) {
    reason = "maxValue must be > minValue";
    return false;
  }

  if (config.render.brightness > 255) {
    config.render.brightness = defaults.render.brightness;
  }

  ParserType parserType = AppDefaults::parserTypeFromString(
      config.mapProfile.parserType,
      ParserType::INDEXED_H1);
  config.mapProfile.parserType = AppDefaults::parserTypeToString(parserType);

    if (config.mapProfile.locationField.isEmpty()) {
      config.mapProfile.locationField = defaults.mapProfile.locationField;
  }
    if (config.mapProfile.valueField.isEmpty()) {
      config.mapProfile.valueField = defaults.mapProfile.valueField;
  }
    if (config.mapProfile.colorField.isEmpty()) {
      config.mapProfile.colorField = defaults.mapProfile.colorField;
  }
  if (config.wifi.hostname.isEmpty()) {
    config.wifi.hostname = defaults.wifi.hostname;
  }

  reason = "ok";
  return true;
}

bool ConfigStore::fromJson(const String& json, AppConfig& outConfig, String& reason) {
  // Config obsahuje zanořené objekty + několik String polí. 4 KB je bezpečný strop.
  StaticJsonDocument<4096> doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    reason = String("deserialize failed: ") + error.c_str();
    return false;
  }

  outConfig = AppDefaults::defaultConfig();

  outConfig.schemaVersion = doc["schemaVersion"] | outConfig.schemaVersion;

  JsonObject wifi = doc["wifi"];
  outConfig.wifi.ssid = String((const char*)(wifi["ssid"] | outConfig.wifi.ssid.c_str()));
  outConfig.wifi.password =
      String((const char*)(wifi["password"] | outConfig.wifi.password.c_str()));
  outConfig.wifi.hostname =
      String((const char*)(wifi["hostname"] | outConfig.wifi.hostname.c_str()));

  JsonObject mapProfile = doc["mapProfile"];
  outConfig.mapProfile.url =
      String((const char*)(mapProfile["url"] | outConfig.mapProfile.url.c_str()));
  outConfig.mapProfile.parserType = String(
      (const char*)(mapProfile["parserType"] | outConfig.mapProfile.parserType.c_str()));
  outConfig.mapProfile.locationField = String(
      (const char*)(mapProfile["locationField"] | outConfig.mapProfile.locationField.c_str()));
  outConfig.mapProfile.valueField = String(
      (const char*)(mapProfile["valueField"] | outConfig.mapProfile.valueField.c_str()));
  outConfig.mapProfile.colorField = String(
      (const char*)(mapProfile["colorField"] | outConfig.mapProfile.colorField.c_str()));
  outConfig.mapProfile.minValue = mapProfile["minValue"] | outConfig.mapProfile.minValue;
  outConfig.mapProfile.maxValue = mapProfile["maxValue"] | outConfig.mapProfile.maxValue;
  outConfig.mapProfile.refreshIntervalMs =
      mapProfile["refreshIntervalMs"] | outConfig.mapProfile.refreshIntervalMs;

  JsonObject render = doc["render"];
  outConfig.render.brightness = render["brightness"] | outConfig.render.brightness;
  outConfig.render.wheelMin = render["wheelMin"] | outConfig.render.wheelMin;
  outConfig.render.wheelMax = render["wheelMax"] | outConfig.render.wheelMax;

  return validateAndNormalize(outConfig, reason);
}

String ConfigStore::toJson(const AppConfig& config) const {
  StaticJsonDocument<4096> doc;

  doc["schemaVersion"] = config.schemaVersion;

  JsonObject wifi = doc.createNestedObject("wifi");
  wifi["ssid"] = config.wifi.ssid;
  wifi["password"] = config.wifi.password;
  wifi["hostname"] = config.wifi.hostname;

  JsonObject mapProfile = doc.createNestedObject("mapProfile");
  mapProfile["url"] = config.mapProfile.url;
  mapProfile["parserType"] = config.mapProfile.parserType;
  mapProfile["locationField"] = config.mapProfile.locationField;
  mapProfile["valueField"] = config.mapProfile.valueField;
  mapProfile["colorField"] = config.mapProfile.colorField;
  mapProfile["minValue"] = config.mapProfile.minValue;
  mapProfile["maxValue"] = config.mapProfile.maxValue;
  mapProfile["refreshIntervalMs"] = config.mapProfile.refreshIntervalMs;

  JsonObject render = doc.createNestedObject("render");
  render["brightness"] = config.render.brightness;
  render["wheelMin"] = config.render.wheelMin;
  render["wheelMax"] = config.render.wheelMax;

  String json;
  serializeJsonPretty(doc, json);

  Serial.println("ConfigStore: serialized config (password hidden)");
  Serial.print("  wifi.ssid: ");
  Serial.println(config.wifi.ssid);
  Serial.print("  wifi.password: ");
  Serial.println(maskedPassword(config.wifi.password));

  return json;
}
