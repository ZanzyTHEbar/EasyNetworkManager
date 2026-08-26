#pragma once
#include <memory>
#include <string>
#include <variant>
#include <vector>
#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_NANO_33_IOT) || \
    defined(ARDUINO_AVR_UNO_WIFI_REV2)
#    include <WiFiNINA.h>
#elif defined(ARDUINO_SAMD_MKR1000)
#    include <WiFi101.h>
#elif defined(ARDUINO_ARCH_ESP8266)
#    include <ESP8266WiFi.h>
#elif defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_NICLA_VISION) || \
    defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_GIGA)
#    include <WiFi.h>
#endif
#include <data/config/project_config.hpp>
#include <data/config/states.hpp>
#include <helpers/helpers.hpp>
#include <helpers/logger.hpp>

using WiFiHandlerCustomHandlerFunction = std::function<void(WiFiState_e event)>;

/* Per-attempt connection timeout driven from loop(); overridable at
 * build time. */
#ifndef EASYNETWORKMANAGER_WIFI_CONNECT_TIMEOUT_MS
#    define EASYNETWORKMANAGER_WIFI_CONNECT_TIMEOUT_MS 15000UL
#endif

class WiFiHandler : public Helpers::Logger,
                    public Helpers::IObserver<StateVariant> {
    WiFiHandlerCustomHandlerFunction customHandlerFunction = NULL;

   public:
    WiFiHandler(ProjectConfig& configManager, const std::string& ssid,
                const std::string& password, uint8_t channel);

    virtual ~WiFiHandler();
    void begin();
    void loop();
    void toggleAdhoc(bool enable);
    void setCustomHandler(
        WiFiHandlerCustomHandlerFunction customHandlerFunction);

    ProjectConfig& configManager;
    Project_Config::WiFiTxPower_t& txpower;

   private:
    void setUpADHOC();
    void adhoc(const std::string& ssid, uint8_t channel,
               const std::string& password = std::string());
    /* Non-blocking connect state machine helpers */
    void startNextAttempt();
    void startAttempt(const std::string& ssid, const std::string& password,
                      uint8_t channel);

    /* Overrides */
    void update(const StateVariant& event) override;

#if defined(ARDUINO_ARCH_ESP32)
    void onWiFiEvent(WiFiEvent_t event);
#elif defined(ARDUINO_ARCH_ESP8266)
    void onStationModeGotIP(
        const WiFiEventStationModeGotIP& event);
    void onStationModeDisconnected(
        const WiFiEventStationModeDisconnected& event);
#endif

    std::string ssid;
    std::string password;
    uint8_t channel;
    uint8_t power;
    bool _enable_adhoc;

    /* Async connect state machine: advanced only from loop(). */
    enum class AsyncConnectState {
        Idle,       /* begin() not called yet */
        Connecting, /* an attempt is in flight */
        Connected,  /* STA associated and has an IP */
        ApFallback  /* all candidates exhausted, AP mode requested */
    };
    AsyncConnectState _connectState;
    unsigned long _attemptStartMs;
    size_t _candidateIndex;
    bool _fallbackAttempted;

#if defined(ARDUINO_ARCH_ESP32)
    WiFiEventId_t _wifiEventId;
#elif defined(ARDUINO_ARCH_ESP8266)
    WiFiEventHandler _gotIpHandler;
    WiFiEventHandler _disconnectedHandler;
#endif
    bool _wifiEventRegistered;
};
