#pragma once
#ifndef UTILITIES_hpp
#define UTILITIES_hpp
#include <Arduino.h>

#include <unordered_map>

#include "wifihandler/WifiHandler.hpp"
namespace Network_Utilities {
bool LoopWifiScan();
void SetupWifiScan();
void my_delay(volatile long delay_time);
int CheckWifiState();
int getStrength(int points);
}  // namespace Network_Utilities
#endif  // !UTILITIES_hpp