#pragma once

#include <Arduino.h>

struct HttpResult {
  bool ok;
  int httpStatus;
  String error;
};

class HttpService {
 public:
  bool get(const String& url, String& payload);
  HttpResult getWithStatus(const String& url, String& payload);
};
