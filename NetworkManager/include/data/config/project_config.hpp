#pragma once
#include <Arduino.h>
#include <utilities/platform_compat.hpp>

#if defined(ESP32)
#    include <Preferences.h>
#elif defined(ESP8266)
#    include "preferences_esp8266.hpp"
#endif

#include "structs.hpp"
#if defined(ESP8266)
#    include "platform_mutex.hpp"
#endif

#include <data/config/config_snapshot_codec.hpp>

#include <deque>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include <data/config/states.hpp>
#include <helpers/helpers.hpp>
#include <helpers/logger.hpp>
#include <helpers/observer.hpp>
#include <utilities/network_utilities.hpp>

enum class ConfigPersistenceAuthority : uint8_t {
    Legacy,
    Snapshot,
    InvalidSnapshot,
};

class CustomConfigInterface {
   public:
    virtual void load() = 0;
    virtual void save() = 0;
};

class ProjectConfig : public Helpers::Logger,
                      public Helpers::ISubject<StateVariant>,
                      public Preferences {
   private:
    virtual void initConfig();
    Project_Config::ProjectConfig_t config;
    std::string _configName;
    std::string _mdnsName;
    bool _already_loaded;
    typedef CustomConfigInterface* _custom_config_interface_t;
    _custom_config_interface_t _custom_config_interface;

    struct PendingNotification {
        bool allObservers;
        uint64_t observerKey;
        StateVariant event;
    };
    std::deque<PendingNotification> pendingNotifications;
#if defined(ESP8266)
    EasyNetworkManagerMutex pendingNotificationMutex;
#else
    std::mutex pendingNotificationMutex;
#endif
    bool notifyingObservers = false;

    void dispatchPendingNotifications();
    void recordPersistenceResult(size_t bytesWritten);
    enum class SnapshotLoadResult : uint8_t { Absent, Valid, Invalid };
    SnapshotLoadResult loadSnapshot(config_snapshot& snapshot);
    void loadLegacyConfig();
    bool persistSnapshot();
    bool persistenceFailed = false;
    ConfigPersistenceAuthority persistenceAuthority =
        ConfigPersistenceAuthority::Legacy;
    bool preferencesOpen = false;

   public:
    ProjectConfig(const std::string& configName = std::string(),
                  const std::string& mdnsName = std::string());
    virtual ~ProjectConfig();
    virtual void load();
    virtual void save();
    bool lastSaveSucceeded() const { return !persistenceFailed; }
    ConfigPersistenceAuthority getPersistenceAuthority() const {
        return persistenceAuthority;
    }
    void wifiConfigSave();
    void deviceConfigSave();
    void mdnsConfigSave();
    void wifiTxPowerConfigSave();
    bool reset();

    // Queue nested notifications so observers never publish while the
    // subject is still dispatching the current event.
    void notifyAll(const StateVariant& event);
    void notify(uint64_t observerKey, const StateVariant& event);

    Project_Config::DeviceConfig_t& getDeviceConfig();
    Project_Config::MDNSConfig_t& getMDNSConfig();
    std::vector<Project_Config::WiFiConfig_t>& getWifiConfigs();
    Project_Config::AP_WiFiConfig_t& getAPWifiConfig();
    Project_Config::WiFiTxPower_t& getWifiTxPowerConfig();
    Project_Config::DeviceDataJson_t& getDeviceDataJson();

    void setDeviceConfig(const std::string& OTAPassword, int OTAPort,
                         bool shouldNotify);
    void setDeviceConfig(const std::string& OTALogin,
                         const std::string& OTAPassword, int OTAPort,
                         bool shouldNotify);
    void setDeviceDataJson(const std::string& deviceJson, bool shouldNotify);
    bool setMDNSConfig(const std::string& mdns, bool shouldNotify);
    void setWifiConfig(const std::string& networkName, const std::string& ssid,
                       const std::string& password, uint8_t channel,
                       uint8_t power, bool adhoc, bool shouldNotify,
                       bool shouldReboot = false);
    void setAPWifiConfig(const std::string& ssid, const std::string& password,
                         uint8_t channel, bool adhoc, bool shouldNotify);
    void setWiFiTxPower(uint8_t power, bool shouldNotify);
    void deleteWifiConfig(const std::string& networkName, bool shouldNotify);

    void registerUserConfig(_custom_config_interface_t custom_config_interface);

    bool reboot;
};
