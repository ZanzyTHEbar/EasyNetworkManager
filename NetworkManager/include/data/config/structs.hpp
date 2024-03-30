#pragma once
#include <string>
#include <vector>

namespace Project_Config {
struct DeviceConfig_t {
    struct Keys_t {
        inline static const std::string ota_login = "ota_login";
        inline static const std::string ota_password = "ota_password";
        inline static const std::string ota_port = "ota_port";
    } keys;

    DeviceConfig_t(const std::string& login, const std::string& password,
                   int port)
        : ota_login(std::move(login)),
          ota_password(std::move(password)),
          ota_port(port) {}

    DeviceConfig_t()
        : ota_login("admin"), ota_password("12345678"), ota_port(3232) {}

    std::string ota_login;
    std::string ota_password;
    int ota_port;
    std::string toRepresentation();
};

struct DeviceDataJson_t {
    struct Keys_t {
        inline static const std::string deviceJson = "deviceJson";
    } keys;

    DeviceDataJson_t(const std::string& json) : deviceJson(std::move(json)) {}

    DeviceDataJson_t() : deviceJson("") {}

    std::string deviceJson;
    std::string toRepresentation();
};

struct MDNSConfig_t {
    struct Keys_t {
        inline static const std::string hostname = "hostname";
    } keys;

    MDNSConfig_t(const std::string& hostname) : hostname(std::move(hostname)) {}

    MDNSConfig_t() : hostname("") {}

    std::string hostname;
    std::string toRepresentation();
};

struct PreferencesWiFiConfigKeys_t {
    inline static const std::string networkCount = "networkCount";
    inline static const std::string name = "name";
    inline static const std::string ssid = "ssid";
    inline static const std::string password = "password";
    inline static const std::string channel = "channel";
    inline static const std::string power = "power";
    inline static const std::string adhoc = "adhoc";
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
    struct Keys_t {
        inline static const std::string power = "power";
    } keys;

    WiFiTxPower_t(uint8_t power) : power(power) {}

    WiFiTxPower_t() : power(52) {}

    uint8_t power;
    std::string toRepresentation();
};

struct AP_WiFiConfig_t {
    struct Keys_t {
        inline static const std::string ssid = "ap_ssid";
        inline static const std::string password = "ap_password";
        inline static const std::string channel = "ap_channel";
        inline static const std::string adhoc = "ap_adhoc";
    } keys;

    AP_WiFiConfig_t(const std::string& ssid, const std::string& password,
                    uint8_t channel, bool adhoc)
        : ssid(std::move(ssid)),
          password(std::move(password)),
          channel(channel),
          adhoc(adhoc) {}

    AP_WiFiConfig_t()
        : ssid("ESP32-AP"), password("12345678"), channel(1), adhoc(false) {}

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

    ProjectConfig_t(const DeviceConfig_t& device,
                    const DeviceDataJson_t& device_data,
                    const MDNSConfig_t& mdns,
                    const std::vector<WiFiConfig_t>& networks,
                    const AP_WiFiConfig_t& ap_network,
                    const WiFiTxPower_t& wifi_tx_power)
        : device(device),
          device_data(device_data),
          mdns(mdns),
          networks(networks),
          ap_network(ap_network),
          wifi_tx_power(wifi_tx_power) {}

    ProjectConfig_t()
        : device(),
          device_data(),
          mdns(),
          networks(),
          ap_network(),
          wifi_tx_power() {}

    PreferencesWiFiConfigKeys_t wifKeys;
};
}  // namespace Project_Config