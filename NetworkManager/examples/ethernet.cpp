#include <Arduino.h>

// Note: Here is a list of all the library header files - required ones are
// marked
//  with an asterisk (*)

//! Optional header files
#include <network/mDNS/MDNSManager.hpp>
#include <network/ota/OTA.hpp>
#include <utilities/network_utilities.hpp>  // various network utilities
// #include <utilities/Observer.hpp>
// #include <utilities/api_utilities.hpp>
// #include <utilities/enuminheritance.hpp> // used for extending enums with new
// values
// #include <utilities/makeunique.hpp> // used with smart pointers (unique_ptr)
// to create unique objects
// #include <utilities/helpers.hpp> // various helper functions

//! Required header files
#define ENABLE_ETHERNET 1        // enable ethernet support
#include <EasyNetworkManager.h>  // (*)

// setup the ethernet connection variables

IPAddress myIP(192, 168, 2, 96);
IPAddress myGW(192, 168, 2, 1);
IPAddress mySN(255, 255, 255, 0);

// Google DNS Server IP
IPAddress myDNS(8, 8, 8, 8);

ProjectConfig configManager;
EthernetManager network(&wifiStateManager, "some_hostname", &myIP, &myGW, &mySN,
                        &myDNS);

APIServer server(80, &configManager, "/api/v1", "/wifimanager",
                 "/userCommands");
OTA ota(&configManager);
MDNSHandler mDNS(&mdnsStateManager, &configManager, "_easynetwork", "test",
                 "_tcp", "_api_port",
                 "80");  //! service name and service protocol have to be
                         //! lowercase and begin with an underscore

void printHelloWorld(AsyncWebServerRequest* request) {
    Serial.println("Hello World!");
}

void blink(AsyncWebServerRequest* request) {
    digitalWrite(27, HIGH);
    Network_Utilities::my_delay(0.5);  // delay for 500ms
    digitalWrite(27, LOW);
    Network_Utilities::my_delay(0.5);  // delay for 500ms
}

void setup() {
    Serial.begin(115200);
    Serial.println("\nHello, EasyNetworkManager!");
    configManager.attach(&mDNS);  // attach the config manager to the mdns
                                  // object - this will update the config
                                  // when mdns hostname changes
    configManager.load();         // load the config from flash

    network.begin();  // setup wifi or ethernet connection
    mDNS.begin();     // start mDNS service (optional)

    // handle the WiFi connection state changes
    switch (wifiStateManager.getCurrentState()) {
        case WiFiState_e::WiFiState_Disconnected: {
            break;
        }
        case WiFiState_e::WiFiState_Disconnecting: {
            break;
        }
        case WiFiState_e::WiFiState_ADHOC: {
            // only start the API server if we have wifi
            // connection
            server.updateCommandHandlers("blink",
                                         blink);  // add a command handler to
                                                  // the API server - you can
                                                  // add as many as you want -
                                                  // you can also add methods.
            server.updateCommandHandlers(
                "helloWorld",
                printHelloWorld);  // add a command handler
                                   // to the API server -
                                   // you can add as many as
                                   // you want - you can
                                   // also add methods.
            server.begin();
            log_d("[SETUP]: Starting API Server");
            break;
        }
        case WiFiState_e::WiFiState_Connected: {
            // only start the API server if we have wifi
            // connection
            server.updateCommandHandlers("blink",
                                         blink);  // add a command handler to
                                                  // the API server - you can
                                                  // add as many as you want -
                                                  // you can also add methods.
            server.updateCommandHandlers(
                "helloWorld",
                printHelloWorld);  // add a command handler
                                   // to the API server -
                                   // you can add as many as
                                   // you want - you can
                                   // also add methods.
            server.begin();
            log_d("[SETUP]: Starting API Server");
            break;
        }
        case WiFiState_e::WiFiState_Connecting: {
            break;
        }
        case WiFiState_e::WiFiState_Error: {
            break;
        }
    }
    ota.begin();
}

void loop() {
    ota.handleOTAUpdate();
}