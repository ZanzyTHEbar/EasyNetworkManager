#pragma once
#ifndef WIFIHANDLER_HPP
#    define WIFIHANDLER_HPP
#    include <memory>
#    include <string>
#    include <vector>
#    if defined(ARDUINO_SAMD_MKRWIFI1010) || \
        defined(ARDUINO_SAMD_NANO_33_IOT) || \
        defined(ARDUINO_AVR_UNO_WIFI_REV2)
#        include <WiFiNINA.h>
#    elif defined(ARDUINO_SAMD_MKR1000)
#        include <WiFi101.h>
#    elif defined(ARDUINO_ARCH_ESP8266)
#        include <ESP8266WiFi.h>
#    elif defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_NICLA_VISION) || \
        defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_GIGA)
#        ifdef USE_ETHERNET
#            include "ethernet/ethernetHandler.hpp"
#        else
#            include <WiFi.h>
#        endif
#    endif
#    include "data/config/project_config.hpp"
#    include "data/statemanager/StateManager.hpp"
#    include "utilities/helpers.hpp"

class WiFiHandler : public IObserver<Event_e> {
   public:
    WiFiHandler(ProjectConfig& configManager, const std::string& ssid,
                const std::string& password, uint8_t channel);

    virtual ~WiFiHandler();
    void begin();
    void toggleAdhoc(bool* enable);

    ProjectConfig& configManager;
    Project_Config::WiFiTxPower_t& txpower;

   private:
    void setUpADHOC();
    void adhoc(const std::string& ssid, uint8_t channel,
               const std::string& password = std::string());
    bool iniSTA(const std::string& ssid, const std::string& password,
                uint8_t channel, wifi_power_t power);

    void update(Event_e event) override;
    std::string getName() override;

    std::string ssid;
    std::string password;
    uint8_t channel;
    uint8_t power;
    bool _enable_adhoc;
};
#endif  // WIFIHANDLER_HPP
