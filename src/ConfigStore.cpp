#include "ConfigStore.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

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
  if (!LittleFS.begin(true)) {
    Serial.println("ConfigStore: LittleFS mount failed");
    return false;
  }

  Serial.println("ConfigStore: LittleFS mounted");
  return true;
}

bool ConfigStore::load(AppConfig& outConfig) {
  AppConfig config = AppDefaults::defaultConfig();

  if (!LittleFS.exists(kConfigPath)) {
    Serial.println("ConfigStore: /config.json missing, using defaults");
    outConfig = config;
    return false;
  }

  File file = LittleFS.open(kConfigPath, "r");
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
  File file = LittleFS.open(kConfigPath, "w");
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

  if (config.dataSource.url.isEmpty()) {
    reason = "dataSource.url empty";
    return false;
  }

  if (config.dataSource.refreshIntervalMs < 100) {
    reason = "refreshIntervalMs too low";
    return false;
  }

  if (config.dataSource.maxValue <= config.dataSource.minValue) {
    reason = "maxValue must be > minValue";
    return false;
  }

  if (config.render.brightness > 255) {
    config.render.brightness = defaults.render.brightness;
  }

  ParserType parserType = AppDefaults::parserTypeFromString(
      config.dataSource.parserType,
      ParserType::INDEXED_H1);
  config.dataSource.parserType = AppDefaults::parserTypeToString(parserType);

  if (config.dataSource.locationField.isEmpty()) {
    config.dataSource.locationField = defaults.dataSource.locationField;
  }
  if (config.dataSource.valueField.isEmpty()) {
    config.dataSource.valueField = defaults.dataSource.valueField;
  }
  if (config.dataSource.colorField.isEmpty()) {
    config.dataSource.colorField = defaults.dataSource.colorField;
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

  JsonObject dataSource = doc["dataSource"];
  outConfig.dataSource.url =
      String((const char*)(dataSource["url"] | outConfig.dataSource.url.c_str()));
  outConfig.dataSource.parserType = String(
      (const char*)(dataSource["parserType"] | outConfig.dataSource.parserType.c_str()));
  outConfig.dataSource.locationField = String(
      (const char*)(dataSource["locationField"] | outConfig.dataSource.locationField.c_str()));
  outConfig.dataSource.valueField = String(
      (const char*)(dataSource["valueField"] | outConfig.dataSource.valueField.c_str()));
  outConfig.dataSource.colorField = String(
      (const char*)(dataSource["colorField"] | outConfig.dataSource.colorField.c_str()));
  outConfig.dataSource.minValue = dataSource["minValue"] | outConfig.dataSource.minValue;
  outConfig.dataSource.maxValue = dataSource["maxValue"] | outConfig.dataSource.maxValue;
  outConfig.dataSource.refreshIntervalMs =
      dataSource["refreshIntervalMs"] | outConfig.dataSource.refreshIntervalMs;

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

  JsonObject dataSource = doc.createNestedObject("dataSource");
  dataSource["url"] = config.dataSource.url;
  dataSource["parserType"] = config.dataSource.parserType;
  dataSource["locationField"] = config.dataSource.locationField;
  dataSource["valueField"] = config.dataSource.valueField;
  dataSource["colorField"] = config.dataSource.colorField;
  dataSource["minValue"] = config.dataSource.minValue;
  dataSource["maxValue"] = config.dataSource.maxValue;
  dataSource["refreshIntervalMs"] = config.dataSource.refreshIntervalMs;

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
