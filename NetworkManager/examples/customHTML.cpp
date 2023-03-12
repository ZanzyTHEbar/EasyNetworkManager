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
#include <EasyNetworkManager.h>  // (*)

ProjectConfig configManager;
WiFiHandler network(&configManager, &wifiStateManager, "", "", 1);

APIServer server(80, &configManager, "/api/v1", "/wifimanager",
                 "/userCommands");
OTA ota(&configManager);  //! Second argument is the Hostname for OTA
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
    Serial.println("Hello, EasyNetworkManager!");
    configManager.attach(
        &mDNS);  // attach the config manager to the mdns object - this will
                 // update the config when mdns hostname changes
    configManager.load();  // load the config from flash

    network.begin();  // setup wifi connection
    mDNS.begin();     // start mDNS service (optional)

    // handle the WiFi connection state changes
    // only start the API server if we have wifi connection
    switch (wifiStateManager.getCurrentState()) {
        case WiFiState_e::WiFiState_Disconnected: {
            break;
        }
        case WiFiState_e::WiFiState_Disconnecting: {
            break;
        }
        case WiFiState_e::WiFiState_ADHOC: {
            // you can add as many as you want
            // The first argument is the route name
            // The second argument is the path to the html file to be served
            // The third argument is the request method (GET, POST, PUT, DELETE,
            // PATCH, OPTIONS)
            server.userEndpointsVector.emplace_back(
                "/hello", "/helloWorld.html",
                "GET");  // add a user endpoint to the vector
            server.userEndpointsVector.emplace_back(
                "/goodbye", "/goodbye.html",
                "POST");  // add a second user endpoint to the vector

            server.updateCommandHandlers(
                "blink",
                blink);  // add a command handler to the API server - you can
                         // add as many as you want - you can also add methods.
            server.updateCommandHandlers(
                "helloWorld", printHelloWorld);  // add a second command handler
                                                 // to the API server
            log_d("[SETUP]: Starting API Server");
            server.begin();  // start the API server
            break;
        }
        case WiFiState_e::WiFiState_Connected: {
            server.userEndpointsVector.emplace_back(
                "/hello", "/helloWorld.html",
                "GET");  // add a user endpoint to the vector
            server.userEndpointsVector.emplace_back(
                "/goodbye", "/goodbye.html",
                "POST");  // add a second user endpoint to the vector

            server.updateCommandHandlers(
                "blink",
                blink);  // add a command handler to the API server - you can
                         // add as many as you want - you can also add methods.
            server.updateCommandHandlers(
                "helloWorld", printHelloWorld);  // add a second command handler
                                                 // to the API server
            server.begin();                      // start the API server
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