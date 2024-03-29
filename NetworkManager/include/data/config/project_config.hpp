#pragma once
#include <Arduino.h>
#include <Preferences.h>

#include <functional>
#include <string>
#include <vector>

#include <utilities/helpers.hpp>
#include <utilities/network_utilities.hpp>
#include <utilities/observer.hpp>
#include <utilities/states.hpp>

namespace Project_Config {
struct DeviceConfig_t {
    std::string ota_login;
    std::string ota_password;
    int ota_port;
    std::string toRepresentation();
};

struct DeviceDataJson_t {
    std::string deviceJson;
    std::string toRepresentation();
};

struct MDNSConfig_t {
    std::string hostname;
    std::string toRepresentation();
};

struct WiFiConfig_t {
    //! Constructor for WiFiConfig_t - allows us to use emplace_back
    WiFiConfig_t(const std::string& name, const std::string& ssid,
                 const std::string& password, uint8_t channel, uint8_t power,
                 bool adhoc)
        : name(std::move(name)),
          ssid(std::move(ssid)),
          password(std::move(password)),
          channel(channel),
          power(power),
          adhoc(adhoc) {}
    std::string name;
    std::string ssid;
    std::string password;
    uint8_t channel;
    uint8_t power;
    bool adhoc;
    std::string toRepresentation();
};

struct WiFiTxPower_t {
    uint8_t power;
    std::string toRepresentation();
};

struct AP_WiFiConfig_t {
    std::string ssid;
    std::string password;
    uint8_t channel;
    bool adhoc;
    std::string toRepresentation();
};

struct ProjectConfig_t {
    DeviceConfig_t device;
    DeviceDataJson_t device_data;
    MDNSConfig_t mdns;
    std::vector<WiFiConfig_t> networks;
    AP_WiFiConfig_t ap_network;
    WiFiTxPower_t wifi_tx_power;
};
}  // namespace Project_Config

class CustomConfigInterface {
   public:
    virtual void load() = 0;
    virtual void save() = 0;
};

class ProjectConfig : public StateManager<StateVariant>, public Preferences {
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
