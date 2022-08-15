#pragma once
#ifndef PROJECT_CONFIG_HPP
#define PROJECT_CONFIG_HPP
#include <Arduino.h>
#include <preferencesAPI.hpp>
#include <vector>
#include <string>

#include "data/utilities/Observer.hpp"

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
        String data_json_string;
        String config_json_string;
        String settings_json_string;
    };

    struct WiFiConfig_t
    {
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

class ProjectConfig : public Config, public ISubject
{
public:
    ProjectConfig(const char *name = (const char *)__null);
    virtual ~ProjectConfig();
    void load();
    void save();
    void reset();
    void initConfig();

    Project_Config::DeviceConfig_t *getDeviceConfig() { return &this->config.device; }
    std::vector<Project_Config::WiFiConfig_t> *getWifiConfigs() { return &this->config.networks; }
    Project_Config::AP_WiFiConfig_t *getAPWifiConfig() { return &this->config.ap_network; }

    void setDeviceConfig(const char *name, const char *OTAPassword, int *OTAPort, bool shouldNotify);
    void setWifiConfig(const char *networkName, const char *ssid, const char *password, uint8_t *channel, bool shouldNotify);
    void setAPWifiConfig(const char *ssid, const char *password, uint8_t *channel, bool shouldNotify);

private:
    const char *configFileName;
    Project_Config::ProjectConfig_t config;
    bool _already_loaded;
    const char *_name;
};

#endif // PROJECT_CONFIG_HPP