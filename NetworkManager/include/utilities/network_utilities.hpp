#pragma once
#ifndef UTILITIES_hpp
#    define UTILITIES_hpp
#    include <Arduino.h>

#    include <unordered_map>

#    include <WiFi.h>
#    include <data/statemanager/StateManager.hpp>
#    include "mbedtls/md.h"

namespace Network_Utilities {
bool loopWifiScan();
void setupWifiScan();
void my_delay(volatile long delay_time);
int getStrength(int points);
std::string generateDeviceID();
void checkWiFiState();
std::string shaEncoder(const std::string& data);
}  // namespace Network_Utilities
#endif  // !UTILITIES_hpp