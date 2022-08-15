#pragma once
#ifndef WIFIHANDLER_HPP
#define WIFIHANDLER_HPP
#include <memory>
#include <string>
#include <WiFi.h>
#include "data/StateManager/StateManager.hpp"
#include "data/config/project_config.hpp"

class WiFiHandler
{
public:
  WiFiHandler(ProjectConfig *configManager, StateManager<WiFiState_e> *stateManager, std::string ssid, std::string password, uint8_t channel);
  virtual ~WiFiHandler();
  void setupWifi();
  void toggleAdhoc(bool *enable);
  
  ProjectConfig *configManager;
  StateManager<WiFiState_e> *stateManager;

private:
  void setUpADHOC();
  void adhoc(const char *ssid, const char *password, uint8_t channel);
  void iniSTA();

  std::string ssid;
  std::string password;
  uint8_t channel;

  bool _enable_adhoc;
};
#endif // WIFIHANDLER_HPP
