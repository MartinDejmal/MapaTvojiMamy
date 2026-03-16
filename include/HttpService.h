#pragma once

#include <Arduino.h>

class HttpService {
 public:
  bool get(const char* url, String& payload);
};
