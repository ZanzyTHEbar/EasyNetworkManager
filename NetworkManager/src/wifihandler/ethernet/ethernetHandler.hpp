#pragma once

#ifndef ETHERNETHANDLER_WT32_ETH01_H
#define ETHERNETHANDLER_WT32_ETH01_H

//////////////////////////////////////////////////////////////

// #if !defined(USING_CORE_ESP32_CORE_V200_PLUS)
#if ((defined(ESP_ARDUINO_VERSION_MAJOR) && \
      (ESP_ARDUINO_VERSION_MAJOR >= 2)) &&  \
     (ARDUINO_ESP32_GIT_VER != 0x46d5afb1))
#define USING_CORE_ESP32_CORE_V200_PLUS true

#if (_ETHERNET_ETHERNETHANDLER_LOGLEVEL_ > 2)
#warning Using code for ESP32 core v2.0.0+ in ETHERNETHANDLER_WT32_ETH01.h
#endif

#define ETHERNETHANDLER_WT32_ETH01_VERSION \
    "ETHERNETHANDLER_WT32_ETH01 v1.5.1 for core v2.0.0+"
#else
#if (_ETHERNET_ETHERNETHANDLER_LOGLEVEL_ > 2)
#warning Using code for ESP32 core v1.0.6- in ETHERNETHANDLER_WT32_ETH01.h
#endif

#define ETHERNETHANDLER_WT32_ETH01_VERSION \
    "ETHERNETHANDLER_WT32_ETH01 v1.5.1 for core v1.0.6-"
#endif

#define ETHERNETHANDLER_WT32_ETH01_VERSION_MAJOR 1
#define ETHERNETHANDLER_WT32_ETH01_VERSION_MINOR 5
#define ETHERNETHANDLER_WT32_ETH01_VERSION_PATCH 1

#define ETHERNETHANDLER_WT32_ETH01_VERSION_INT 1005001

#if ESP32
#warning Using ESP32 architecture for ETHERNETHANDLER_WT32_ETH01
#define BOARD_NAME "WT32-ETH01"
#else
#error This code is designed to run on ESP32 platform! Please check your Tools->Board setting.
#endif
#include <Arduino.h>

#include <string>

// defined here before #include <ETH.h> to override

// I²C-address of Ethernet PHY (0 or 1 for LAN8720, 31 for
// TLK110)
#ifndef ETH_PHY_ADDR
#define ETH_PHY_ADDR 1
#endif

// Type of the Ethernet PHY (LAN8720 or TLK110)
// typedef enum { ETH_PHY_LAN8720, ETH_PHY_TLK110,
// ETH_PHY_RTL8201, ETH_PHY_DP83848, ETH_PHY_DM9051,
// ETH_PHY_KSZ8081, ETH_PHY_MAX } eth_phy_type_t;

#ifndef ETH_PHY_TYPE
#define ETH_PHY_TYPE ETH_PHY_LAN8720
#endif

// Pin# of the enable signal for the external crystal
// oscillator (-1 to disable for internal APLL source)
#ifndef ETH_PHY_POWER
#define ETH_PHY_POWER 16
#endif

// Pin# of the I²C clock signal for the Ethernet PHY
#ifndef ETH_PHY_MDC
#define ETH_PHY_MDC 23
#endif

// Pin# of the I²C IO signal for the Ethernet PHY
#ifndef ETH_PHY_MDIO
#define ETH_PHY_MDIO 18
#endif

/*
  //typedef enum { ETH_CLOCK_GPIO0_IN, ETH_CLOCK_GPIO0_OUT,
  ETH_CLOCK_GPIO16_OUT, ETH_CLOCK_GPIO17_OUT }
  eth_clock_mode_t; ETH_CLOCK_GPIO0_IN   - default: external
  clock from crystal oscillator ETH_CLOCK_GPIO0_OUT  - 50MHz
  clock from internal APLL output on GPIO0 - possibly an
  inverter is needed for LAN8720 ETH_CLOCK_GPIO16_OUT -
  50MHz clock from internal APLL output on GPIO16 - possibly
  an inverter is needed for LAN8720 ETH_CLOCK_GPIO17_OUT -
  50MHz clock from internal APLL inverted output on GPIO17 -
  tested with LAN8720
*/
#ifndef ETH_CLK_MODE
#define ETH_CLK_MODE \
    ETH_CLOCK_GPIO0_IN  //  ETH_CLOCK_GPIO17_OUT
#endif

#include <ETH.h>
#include <WiFi.h>

#ifndef SHIELD_TYPE
#define SHIELD_TYPE "ETH_PHY_LAN8720"
#endif

class EthernetHandler {
   private:
    bool _WT32_ETH01_eth_connected;  // false;
    std::string hostName;

   public:
    EthernetHandler(const std::string &hostName);
    virtual ~EthernetHandler();
    void WT32_ETH01_onEvent();
    void WT32_ETH01_waitForConnect();
    bool WT32_ETH01_isConnected();
    static void WT32_ETH01_event(WiFiEvent_t event);
};

#endif  // ETHERNETHANDLER_WT32_ETH01_H
