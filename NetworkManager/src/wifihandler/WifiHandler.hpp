#pragma once
#ifndef WIFIHANDLER_HPP
#define WIFIHANDLER_HPP
#include <memory>
#include <string>
#include <vector>

#ifdef ESP32
#ifdef USE_ETHERNET
#include "ethernet/ethernetHandler.hpp"
#else
#include <WiFi.h>
#endif

#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include "data/StateManager/StateManager.hpp"
#include "data/config/project_config.hpp"
#include "data/utilities/helpers.hpp"

class WiFiHandler {
   public:
    WiFiHandler(ProjectConfig *configManager, StateManager<WiFiState_e> *stateManager, const std::string &ssid,
                const std::string &password, uint8_t channel
#ifdef USE_ETHERNET
                ,
                IPAddress *ethIP, IPAddress *ethGW, IPAddress *ethSN, IPAddress *ethDNS
#endif
    );

    virtual ~WiFiHandler();
    void setupWifi();
    void toggleAdhoc(bool *enable);

    ProjectConfig *configManager;
    StateManager<WiFiState_e> *stateManager;
    Project_Config::WiFiTxPower_t *txpower;

   private:
    void setUpADHOC();
    void adhoc(const std::string &ssid, uint8_t channel, const std::string &password = std::string());
    bool iniSTA(const std::string &ssid, const std::string &password, uint8_t channel, wifi_power_t power);

    std::string ssid;
    std::string password;
    uint8_t channel;
    uint8_t power;
    bool _enable_adhoc;
#ifdef USE_ETHERNET
    // Device IP Address
    IPAddress *ethIP;
    // Gateway IP Address
    IPAddress *ethGW;
    // Subnet Mask
    IPAddress *ethSN;
    // Google DNS Server IP
    IPAddress *ethDNS;
#endif
};
#endif  // WIFIHANDLER_HPP
