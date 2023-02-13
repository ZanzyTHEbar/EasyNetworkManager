#pragma once
#ifndef UTILITIES_hpp
#    define UTILITIES_hpp
#    include <Arduino.h>

#    include <unordered_map>

#    include "network/wifihandler/WifiHandler.hpp"

namespace Network_Utilities {
bool LoopWifiScan();
void SetupWifiScan();
void my_delay(volatile long delay_time);
int CheckWifiState();
int getStrength(int points);
std::string generateDeviceID();
}  // namespace Network_Utilities
#endif  // !UTILITIES_hpp