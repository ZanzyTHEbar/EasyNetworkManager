#include <Arduino.h>

//? Here is a list of all the library header files - required ones are marked
// with an asterisk (*)

//! Optional header files
#include <mDNS/MDNSManager.hpp>
#include <ota/OTA.hpp>
// #include <memory>
// #include <data/utilities/Observer.hpp>
// #include <api/utilities/apiUtilities.hpp>
// #include <data/utilities/enuminheritance.hpp> // used for extending enums
// with new values #include <data/utilities/makeunique.hpp> // used with smart
// pointers (unique_ptr) to create unique objects #include
// <data/utilities/helpers.hpp> // various helper functions
#include <data/utilities/network_utilities.hpp>  // various network utilities
// #include <wifihandler/utilities/utilities.hpp> // various wifi related
// utilities

//! Required header files
#include <api/webserverHandler.hpp>            //! (*)
#include <data/StateManager/StateManager.hpp>  //! (*)
#include <data/config/project_config.hpp>      //! (*)
#include <wifihandler/wifiHandler.hpp>         //! (*)

//? The Project Config Manager is used to store and retrieve the configuration
// data ? The config manager constructor takes two (optional) parameters: ? 1.
// The name of the project (used to create the config file name) ? 2. The
// hostname for your device on the network(used for mDNS, OTA, etc.)
ProjectConfig configManager("easynetwork", "esp32custom");
//? The WiFi Handler is used to manage the WiFi connection
//? The WiFi Handler constructor takes four parameters:
//? 1. A pointer to the config manager
//? 2. A pointer to the WiFi State Manager
//? 3. The SSID of the WiFi network to connect to
//? 4. The password of the WiFi network to connect to
WiFiHandler network(&configManager, &wifiStateManager, "YOUR_SSID", "YOUR_PASWORD", 1);

//? The API Server is used to create a web server that can be used to send
// commands to the device ? The API Server constructor takes five parameters:
//? 1. The port number to use for the web server
//? 2. A pointer to the config manager
//? 3. The root path for the API
//? 4. The path for the WiFi Manager html page
//? 5. The root path for the user commands for the API
//? This file example will create a web server on port 80, with the root path:
// http://easynetwork.local/api/mycommands/command/helloWorld
// http://easynetwork.local/api/mycommands/command/blink
// http://easynetwork.local/api/mycommands/command/params?Axes1=1&Axes2=2
APIServer server(80, &configManager, "/api", "/wifimanager",
                 "/mycommands");
OTA ota(&configManager);

//? The mDNS Manager is used to create a mDNS service for the device
//? The mDNS Manager constructor takes seven parameters:
//? 1. A pointer to the mDNS State Manager
//? 2. A pointer to the config manager
//? 3. The service name
//? 4. The service instance name
//? 5. The service protocol
//? 6. The service description
//? 7. The service port
MDNSHandler mDNS(&mdnsStateManager, &configManager, "_easynetwork", "test",
                 "_tcp", "_api_port",
                 "80");  //! service name and service protocol have to be
                         //! lowercase and begin with an underscore

//? Here is a function that can be used to handle custom API requests
void grabParams(AsyncWebServerRequest* request) {
    int params = request->params();
    log_d("Number of Params: %d", params);
    std::string y;
    std::string x;
    for (int i = 0; i < params; i++) {
        AsyncWebParameter* param = request->getParam(i);
        if (param->name() == "Axes1") {
            x = param->value().c_str();
        } else if (param->name() == "Axes2") {
            y = param->value().c_str();
        }
    }
    Serial.printf("Axes1: %s, Axes2: %s\n", x.c_str(), y.c_str());
    request->send(200, "text/plain", "OK");
}

//? Here are two functions that can be used to handle custom API requests,
// without access to the request object
void printHelloWorld() {
    Serial.println("Hello World!");
}

void blink() {
    digitalWrite(27, HIGH);
    Network_Utilities::my_delay(0.5);  // delay for 500ms
    digitalWrite(27, LOW);
    Network_Utilities::my_delay(0.5);  // delay for 500ms
}

void setupServer() {
    // add command handlers to the API server
    // you can add as many as you want - you can also add methods.
    log_d("[SETUP]: Starting API Server");
    server.updateCommandHandlers("blink", blink);
    server.updateCommandHandlers("helloWorld", printHelloWorld);
    server.updateCommandHandlers("params", grabParams);
    server.begin();
    log_d("[SETUP]: API Server Started");
}

void setup() {
    Serial.begin(115200);
    Serial.println("\nHello, EasyNetworkManager!");

    Serial.setDebugOutput(true);
    configManager.initConfig();  // call before load to initialise the structs
    configManager.attach(
        &mDNS);  // attach the config manager to the mdns object - this will
                 // update the config when mdns hostname changes
    configManager.load();  // load the config from flash

    network.begin();   // setup wifi connection
    mDNS.startMDNS();  // start mDNS service (optional)

    // handle the WiFi connection state changes
    switch (wifiStateManager.getCurrentState()) {
        case WiFiState_e::WiFiState_Disconnected: {
            break;
        }
        case WiFiState_e::WiFiState_Disconnecting: {
            break;
        }
        case WiFiState_e::WiFiState_ADHOC: {
            // only start the API server if we have AP connection
            setupServer();
            break;
        }
        case WiFiState_e::WiFiState_Connected: {
            // only start the API server if we have wifi connection
            setupServer();
            break;
        }
        case WiFiState_e::WiFiState_Connecting: {
            break;
        }
        case WiFiState_e::WiFiState_Error: {
            break;
        }
    }
    ota.SetupOTA();
}

void loop() {
    ota.HandleOTAUpdate();
}