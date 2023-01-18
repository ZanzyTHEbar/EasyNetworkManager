#include "ethernetHandler.hpp"

EthernetManager::EthernetManager(StateManager<WiFiState_e>* stateManager,
                                 const std::string& hostName, IPAddress* ethIP,
                                 IPAddress* ethGW, IPAddress* ethSN,
                                 IPAddress* ethDNS)
    : _WT32_ETH01_eth_connected(false),
      hostName(std::move(hostName)),
      ethIP(ethIP),
      ethGW(ethGW),
      ethSN(ethSN),
      ethDNS(ethDNS) {}

EthernetManager::~EthernetManager() {}

void EthernetManager::WT32_ETH01_onEvent() {
    WiFi.onEvent(std::bind(&EthernetManager::WT32_ETH01_event, this,
                           std::placeholders::_1));
}

void EthernetManager::WT32_ETH01_waitForConnect() {
    while (!_WT32_ETH01_eth_connected)
        delay(100);
}

bool EthernetManager::WT32_ETH01_isConnected() {
    return _WT32_ETH01_eth_connected;
}

void EthernetManager::WT32_ETH01_event(WiFiEvent_t event) {
    switch (event) {
        // #if USING_CORE_ESP32_CORE_V200_PLUS
#if ((defined(ESP_ARDUINO_VERSION_MAJOR) && \
      (ESP_ARDUINO_VERSION_MAJOR >= 2)) &&  \
     (ARDUINO_ESP32_GIT_VER != 0x46d5afb1))
        // For breaking core v2.0.0
        // Why so strange to define a breaking enum
        // arduino_event_id_t in WiFiGeneric.h compared to
        // the old system_event_id_t, now in
        // tools/sdk/esp32/include/esp_event/include/esp_event_legacy.h
        // You can preserve the old enum order and just
        // adding new items to do no harm
        case ARDUINO_EVENT_ETH_START:
            log_i("\nETH Started");
            // set eth hostname here
            ETH.setHostname(hostName.c_str());
            break;
        case ARDUINO_EVENT_ETH_CONNECTED:
            log_i("ETH Connected");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            if (!_WT32_ETH01_eth_connected) {
                log_i("ETH MAC: %s, IPv4: %s", ETH.macAddress().c_str(),
                      ETH.localIP().toString().c_str());
                log_i("%s",
                      ETH.fullDuplex() ? "FULL_DUPLEX, " : "HALF_DUPLEX, ");
                log_i("%d Mbps", ETH.linkSpeed());
                _WT32_ETH01_eth_connected = true;
            }
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            log_i("ETH Disconnected");
            _WT32_ETH01_eth_connected = false;
            break;
        case ARDUINO_EVENT_ETH_STOP:
            log_i("\nETH Stopped");
            _WT32_ETH01_eth_connected = false;
            break;
#else
        // For old core v1.0.6-
        // Core v2.0.0 defines a stupid enum
        // arduino_event_id_t, breaking any code for
        // WT32_ETH01 written for previous core Why so
        // strange to define a breaking enum
        // arduino_event_id_t in WiFiGeneric.h compared to
        // the old system_event_id_t, now in
        // tools/sdk/esp32/include/esp_event/include/esp_event_legacy.h
        // You can preserve the old enum order and just
        // adding new items to do no harm
        case SYSTEM_EVENT_ETH_START:
            log_i("\nETH Started");
            // set eth hostname here
            ETH.setHostname(hostName.c_str());
            break;
        case SYSTEM_EVENT_ETH_CONNECTED:
            log_i(F("ETH Connected"));
            break;
        case SYSTEM_EVENT_ETH_GOT_IP:
            if (!_WT32_ETH01_eth_connected) {
                log_i("ETH MAC: %s, IPv4: %s", ETH.macAddress().c_str(),
                      ETH.localIP().toString().c_str());
                log_i("%s",
                      ETH.fullDuplex() ? "FULL_DUPLEX, " : "HALF_DUPLEX, ");
                log_i("%d Mbps", ETH.linkSpeed());
                _WT32_ETH01_eth_connected = true;
            }
            break;
        case SYSTEM_EVENT_ETH_DISCONNECTED:
            log_i("ETH Disconnected");
            _WT32_ETH01_eth_connected = false;
            break;
        case SYSTEM_EVENT_ETH_STOP:
            log_i("\nETH Stopped");
            _WT32_ETH01_eth_connected = false;
            break;
#endif
        default:
            break;
    }
}

void EthernetManager::begin() {
    WT32_ETH01_onEvent();
    ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER);
    ETH.config(*ethIP, *ethGW, *ethSN, *ethDNS);
    WT32_ETH01_waitForConnect();
    if (WT32_ETH01_isConnected()) {
        WiFi.softAP();
        stateManager->setState(WiFiState_e::WiFiState_Connected);
        return;
    }
    log_e("Ethernet connection failed!");
    return;
}