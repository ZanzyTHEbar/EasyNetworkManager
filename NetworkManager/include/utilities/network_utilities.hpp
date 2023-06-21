#pragma once
#ifndef UTILITIES_hpp
#    define UTILITIES_hpp
#    include <Arduino.h>

#    include <unordered_map>

#    include <WiFi.h>
#    include <data/statemanager/state_manager.hpp>

namespace Network_Utilities {
bool loopWifiScan();
void setupWifiScan();
void my_delay(volatile long delay_time);
int getStrength(int points);
std::string generateDeviceID();
void checkWiFiState();
}  // namespace Network_Utilities
#endif  // !UTILITIES_hpp