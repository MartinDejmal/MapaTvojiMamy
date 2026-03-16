#pragma once

#include <Arduino.h>

class HttpService {
 public:
  bool get(const String& url, String& payload);
};
