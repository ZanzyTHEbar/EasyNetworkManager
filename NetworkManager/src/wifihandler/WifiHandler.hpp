#pragma once
#ifndef WIFIHANDLER_HPP
#define WIFIHANDLER_HPP
#include <memory>
#include <string>
#include <WiFi.h>
#include "data/StateManager/StateManager.hpp"
#include "data/config/project_config.hpp"
#include "data/utilities/helpers.hpp"

class WiFiHandler
{
public:
  WiFiHandler(ProjectConfig *configManager,
              StateManager<WiFiState_e> *stateManager,
              const std::string &ssid,
              const std::string &password,
              uint8_t channel);
  virtual ~WiFiHandler();
  void setupWifi();
  void toggleAdhoc(bool *enable);

  ProjectConfig *configManager;
  StateManager<WiFiState_e> *stateManager;
  Project_Config::WiFiTxPower_t *txpower;

private:
  void setUpADHOC();
  void adhoc(const char *ssid, uint8_t channel, const char *password = NULL);
  bool iniSTA(const char *ssid, const char *password, uint8_t channel, wifi_power_t power);

  std::string ssid;
  std::string password;
  uint8_t channel;
  uint8_t power;
  bool _enable_adhoc;
};
#endif // WIFIHANDLER_HPP
