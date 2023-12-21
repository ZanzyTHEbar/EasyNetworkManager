#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <unordered_map>

namespace Network_Utilities {
bool loopWifiScan();
void setupWifiScan();
void my_delay(volatile long delay_time);
int getStrength(int points);
std::string generateDeviceID();
void checkWiFiState();
}  // namespace Network_Utilities