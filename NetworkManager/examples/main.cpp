#include <Arduino.h>
#include <data/config/project_config.hpp>
#include <data/StateManager/StateManager.hpp>
#include <data/utilities/Observer.hpp>
#include <wifihandler/wifiHandler.hpp>
#include <api/webserverHandler.hpp>
#include <mDNS/MDNSManager.hpp>
#include <ota/OTA.hpp>
//#include <data/utilities/enuminheritance.hpp> // used for extending enums with new values
//#include <data/utilities/makeunique.hpp> // used with smart pointers (unique_ptr) to create unique objects

ProjectConfig configManager("Network");
WiFiHandler network(&configManager, &wifiStateManager, "ssid", "password", 1);
APIServer server(80, &network, "/control", "/wifimanager");
OTA ota(&configManager);
MDNSHandler mDNS(&mdnsStateManager, &configManager, "EasyNetworkManager", "test", "tcp", "api_port", "80");

LEDManager ledManager(27);

void printHelloWorld()
{
    Serial.println("Hello World!");
}

void blink()
{
    digitalWrite(27, HIGH);
    delay(100);
    digitalWrite(27, LOW);
    delay(100);
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Hello, EasyNetworkManager!");
    ledManager.begin();

    Serial.setDebugOutput(true);
    configManager.initConfig(); // call before load to initialise the structs
    configManager.load();       // load the config from flash

    network.setupWifi(); // setup wifi connection
    mDNS.startMDNS();    // start mDNS service (optional)

    // handle the WiFi connection state changes
    switch (wifiStateManager.getCurrentState())
    {
    case WiFiState_e::WiFiState_Disconnected:
    {
        break;
    }
    case WiFiState_e::WiFiState_Disconnecting:
    {
        break;
    }
    case WiFiState_e::WiFiState_ADHOC:
    {
        // do not put a break here, as we want to fall through to the next case
    }
    case WiFiState_e::WiFiState_Connected:
    {
        // only start the API server if we have wifi connection
        server.addCommandHandler("hello_world", printHelloWorld); // add a command handler to the API server - you can add as many as you want - you can also add methods.
        server.addCommandHandler("blink", blink);                 // add a command handler to the API server - you can add as many as you want - you can also add methods.

        server.begin();
        log_d("[SETUP]: Starting API Server");
        break;
    }
    case WiFiState_e::WiFiState_Connecting:
    {
        break;
    }
    case WiFiState_e::WiFiState_Error:
    {
        break;
    }
    }
    ota.SetupOTA();
}

void loop()
{
    ota.HandleOTAUpdate();
    server.triggerWifiConfigWrite();
}