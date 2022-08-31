#pragma once
#ifndef PROJECT_CONFIG_HPP
#define PROJECT_CONFIG_HPP
#include <Arduino.h>
#include <Preferences.h>
#include <vector>
#include <string>

#include "data/utilities/Observer.hpp"
#include "data/utilities/helpers.hpp"

namespace Project_Config
{
    struct DeviceConfig_t
    {
        std::string name;
        std::string OTAPassword;
        int OTAPort;
        bool data_json;
        bool config_json;
        bool settings_json;
        std::string data_json_string;
        std::string config_json_string;
        std::string settings_json_string;
    };

    struct WiFiConfig_t
    {
        //! Constructor for WiFiConfig_t - allows us to use emplace_back
        WiFiConfig_t(const std::string &name,
                     const std::string &ssid,
                     const std::string &password,
                     uint8_t channel) : name(std::move(name)),
                                        ssid(std::move(ssid)),
                                        password(std::move(password)),
                                        channel(channel) {}
        std::string name;
        std::string ssid;
        std::string password;
        uint8_t channel;
    };

    struct AP_WiFiConfig_t
    {
        std::string ssid;
        std::string password;
        uint8_t channel;
    };

    struct ProjectConfig_t
    {
        DeviceConfig_t device;
        std::vector<WiFiConfig_t> networks;
        AP_WiFiConfig_t ap_network;
    };
}

class ProjectConfig : public Preferences, public ISubject
{
public:
    ProjectConfig(const std::string &name = std::string());
    virtual ~ProjectConfig();
    void load();
    void save();
    bool reset();
    void initConfig();

    Project_Config::DeviceConfig_t *getDeviceConfig() { return &this->config.device; }
    std::vector<Project_Config::WiFiConfig_t> *getWifiConfigs() { return &this->config.networks; }
    Project_Config::AP_WiFiConfig_t *getAPWifiConfig() { return &this->config.ap_network; }

    void setDeviceConfig(const std::string &name, const std::string &OTAPassword, int *OTAPort, bool shouldNotify);
    void setWifiConfig(const std::string &networkName, const std::string &ssid, const std::string &password, uint8_t *channel, bool shouldNotify);
    void setAPWifiConfig(const std::string &ssid, const std::string &password, uint8_t *channel, bool shouldNotify);

private:
    Project_Config::ProjectConfig_t config;
    bool _already_loaded;
    std::string _name;
};

#endif // PROJECT_CONFIG_HPP