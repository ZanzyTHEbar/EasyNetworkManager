#pragma once

#if defined(ESP8266)

#include <Arduino.h>

// ESP8266 has no ESP-IDF log macros; keep the existing messages on Serial.
#ifndef log_e
#    define log_e(...) Serial.printf(__VA_ARGS__)
#endif
#ifndef log_w
#    define log_w(...) Serial.printf(__VA_ARGS__)
#endif
#ifndef log_i
#    define log_i(...) Serial.printf(__VA_ARGS__)
#endif
#ifndef log_d
#    define log_d(...) Serial.printf(__VA_ARGS__)
#endif
#ifndef log_v
#    define log_v(...) Serial.printf(__VA_ARGS__)
#endif

#endif  // ESP8266
