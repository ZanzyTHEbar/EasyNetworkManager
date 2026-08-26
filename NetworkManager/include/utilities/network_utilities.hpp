#pragma once
#include <Arduino.h>
#include <utilities/platform_compat.hpp>
#if defined(ESP8266)
#    include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <unordered_map>
#include <string>

namespace Network_Utilities {
bool loopWifiScan();
void setupWifiScan();
void my_delay(volatile long delay_time);
int getStrength(int points);
std::string generateDeviceID();
void checkWiFiState();
}  // namespace Network_Utilities
