#include <Arduino.h>
#include <data/config/project_config.hpp>
#include <data/StateManager/StateManager.hpp>
#include <data/utilities/Observer.hpp>
#include <wifihandler/wifiHandler.hpp>
#include <api/utilities/apiUtilities.hpp>
#include <api/webserverHandler.hpp>
#include <mDNS/MDNSManager.hpp>
#include <ota/OTA.hpp>
//#include <memory>
//#include <data/utilities/enuminheritance.hpp> // used for extending enums with new values
//#include <data/utilities/makeunique.hpp> // used with smart pointers (unique_ptr) to create unique objects

ProjectConfig configManager("network");
WiFiHandler network(&configManager, &wifiStateManager, "SSID", "PASS", "_easynetworkmanager", 1);

//? NULL is required, it is a placeholder for the DNSServer - which is not yet fully implemented.
//? The DNSServer is used to redirect the user to the captive portal when they connect to the network. Will be implemented soon.
//! The routes require a leading forward slash, as show here
APIServer server(80, &network, NULL, "/api/v1", "/wifimanager"); 
OTA ota(&configManager);
MDNSHandler mDNS(&mdnsStateManager, &configManager, "_easynetworkmanager", "test", "_tcp", "api_port", "80"); //! service name and service protocol have to be lowercase and begin with an underscore

void printHelloWorld()
{
    Serial.println("Hello World!");
}

// Will implement an example of how to set callbacks in the future
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
        // To add new commands to the command handler 
        // - create a class and inherit from the APIServer class or BaseAPI class 
        // - pass in your params to the constructor as show in APIServer class
        // - then create your methods and add them to the command map - see APIServer class method setupServer() for example
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