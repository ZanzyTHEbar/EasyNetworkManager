#pragma once

#include <ArduinoJson.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <data/config/structs.hpp>

// Pure JSON codec for the persisted config snapshot. Deliberately free of
// Arduino types so it can be unit-tested on the host.
struct config_snapshot {
    static constexpr uint8_t current_version = 1;

    uint8_t version = current_version;
    Project_Config::DeviceConfig_t device;
    Project_Config::DeviceDataJson_t device_data;
    Project_Config::MDNSConfig_t mdns;
    Project_Config::WiFiTxPower_t wifi_tx_power;
    Project_Config::AP_WiFiConfig_t ap_network;
    std::vector<Project_Config::WiFiConfig_t> networks;
};

constexpr size_t kMaxSavedNetworks = 3;

// ESP32 NVS strings include their terminating null byte in the 4000-byte
// limit. Keep the shared snapshot writer within that limit on both platforms.
constexpr size_t kMaxSnapshotJsonBytes = 3999;

bool parseSnapshot(const std::string& serialized, config_snapshot& snapshot);

bool serializeSnapshot(const config_snapshot& snapshot,
                       std::string& serialized);

std::string jsonEscape(const std::string& value);

bool validOtaPort(int port);

bool validWifiChannel(uint8_t channel);

bool validWifiPower(uint8_t power);

bool validHostname(const std::string& hostname);
