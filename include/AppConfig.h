#pragma once

namespace AppConfig {

constexpr const char* WIFI_SSID = "";
constexpr const char* WIFI_PASSWORD = "";
constexpr const char* JSON_URL = "http://tmep.cz/vystup-json.php?okresy_cr=1";

constexpr int LED_COUNT = 77;
constexpr int LED_PIN = 27;
constexpr int LED_CHANNEL = 0;
constexpr int LED_BRIGHTNESS = 10;

constexpr unsigned long FETCH_INTERVAL_MS = 1000;

constexpr int TEMP_MIN = -15;
constexpr int TEMP_MAX = 40;
constexpr int WHEEL_MIN = 170;
constexpr int WHEEL_MAX = 0;

}  // namespace AppConfig
