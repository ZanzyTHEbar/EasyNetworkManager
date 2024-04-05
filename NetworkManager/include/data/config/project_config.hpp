#pragma once
#include <Arduino.h>
#include <Preferences.h>

#include "structs.hpp"

#include <functional>
#include <string>
#include <vector>

#include <data/config/states.hpp>
#include <helpers/helpers.hpp>
#include <helpers/observer.hpp>
#include <utilities/network_utilities.hpp>

class CustomConfigInterface {
   public:
    virtual void load() = 0;
    virtual void save() = 0;
};

class ProjectConfig : public Helpers::ISubject<StateVariant>, public Preferences {
   private:
    virtual void initConfig();
    Project_Config::ProjectConfig_t config;
    std::string _configName;
    std::string _mdnsName;
    bool _already_loaded;
    typedef CustomConfigInterface* _custom_config_interface_t;
    _custom_config_interface_t _custom_config_interface;

   public:
    ProjectConfig(const std::string& configName = std::string(),
                  const std::string& mdnsName = std::string());
    virtual ~ProjectConfig();
    virtual void load();
    virtual void save();
    void wifiConfigSave();
    void deviceConfigSave();
    void mdnsConfigSave();
    void wifiTxPowerConfigSave();
    bool reset();

    Project_Config::DeviceConfig_t& getDeviceConfig();
    Project_Config::MDNSConfig_t& getMDNSConfig();
    std::vector<Project_Config::WiFiConfig_t>& getWifiConfigs();
    Project_Config::AP_WiFiConfig_t& getAPWifiConfig();
    Project_Config::WiFiTxPower_t& getWifiTxPowerConfig();
    Project_Config::DeviceDataJson_t& getDeviceDataJson();

    void setDeviceConfig(const std::string& OTAPassword, int OTAPort,
                         bool shouldNotify);
    void setDeviceDataJson(const std::string& deviceJson, bool shouldNotify);
    bool setMDNSConfig(const std::string& mdns, bool shouldNotify);
    void setWifiConfig(const std::string& networkName, const std::string& ssid,
                       const std::string& password, uint8_t channel,
                       uint8_t power, bool adhoc, bool shouldNotify);
    void setAPWifiConfig(const std::string& ssid, const std::string& password,
                         uint8_t channel, bool adhoc, bool shouldNotify);
    void setWiFiTxPower(uint8_t power, bool shouldNotify);
    void deleteWifiConfig(const std::string& networkName, bool shouldNotify);

    void registerUserConfig(_custom_config_interface_t custom_config_interface);

    bool reboot;
};
