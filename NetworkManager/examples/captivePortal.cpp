#include <Arduino.h>

// Note: Here is a list of all the library header files - required ones are
// marked
//  with an asterisk (*)

//! This is a Captive Portal Example. Please see the customHTML.cpp for an example of how to implement custom HTML files to use instead of the default wifimanager.

//! Optional header files
#include <DNSServer.h>  // (*) used for captive portal
#include <network/mdns/mdns_manager.hpp>
#include <network/ota/basic_ota.hpp>
#include <utilities/network_utilities.hpp>  // various network utilities
// (unique_ptr) to create unique objects
// #include <utilities/Observer.hpp>
// #include <utilities/api_utilities.hpp>
// #include <utilities/enuminheritance.hpp> // used for extending enums with new
// values
// #include <utilities/makeunique.hpp> // used with smart pointers (unique_ptr)
// to create unique objects
// #include <utilities/helpers.hpp> // various helper functions

//! Required header files
#include <EasyNetworkManager.h>  // (*)

// Note: The Project Config Manager is used to store and retrieve the
// configuration
//  data ? The config manager constructor takes two (optional) parameters: ? 1.
//  The name of the project (used to create the config file name) ? 2. The
//  hostname for your device on the network(used for mDNS, OTA, etc.)
ConfigHandler configHandler("easynetwork", MDNS_HOSTNAME);

// Note: The WiFi Handler is used to manage the WiFi connection
// Note: The WiFi Handler constructor takes four parameters:
// Note: 1. A pointer to the config manager
// Note: 2. A pointer to the WiFi State Manager
// Note: 3. The SSID of the WiFi network to connect to
// Note: 4. The password of the WiFi network to connect to
WiFiHandler network(configHandler.config, WIFI_SSID, WIFI_PASSWORD, 1);

DNSServer dnsServer;

// Note: The API Server is used to create a web server that can be used to send
//  commands to the device ? The API Server constructor takes five parameters:
// Note: 1. The port number to use for the web server
// Note: 2. A pointer to the config manager
// Note: 3. The root path for the API
// Note: 4. The path for the WiFi Manager html page
// Note: 5. The root path for the user commands for the API
// Note: This file example will create a web server on port 80, with the root
// path:
//  http://easynetwork.local/api/mycommands/command/helloWorld
//  http://easynetwork.local/api/mycommands/command/blink
//  http://easynetwork.local/api/mycommands/command/params?Axes1=1&Axes2=2
APIServer server(80, configHandler.config, "/api", "/wifimanager",
                 "/mycommands");

// Note: Not required if you are going to use the AsyncOTA feature
OTA ota(configHandler.config);

// Note: The mDNS Manager is used to create a mDNS service for the device
// Note: The mDNS Manager constructor takes seven parameters:
// Note: 1. A pointer to the mDNS State Manager
// Note: 2. A pointer to the config manager
// Note: 3. The service name
// Note: 4. The service instance name
// Note: 5. The service protocol
// Note: 6. The service description
// Note: 7. The service port

//! service name and service protocol have to be
//! lowercase and begin with an underscore
MDNSHandler mDNS(configHandler.config, "_easynetwork", "test", "_tcp",
                 "_api_port", "80");

class CustomConfig : public CustomConfigInterface {
    void save() override {
        Serial.println("Saving custom config");
    }

    void load() override {
        Serial.println("Loading custom config");
    }
};

CustomConfig customConfig;

class Temp {
   public:
    Temp() {
        Serial.println("Temp created");
    }
    ~Temp() {
        Serial.println("Temp destroyed");
    }
    // Note: Here is a function that can be used to handle custom API requests
    // inside Note: of a class
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
};

// Note: Here is a function that can be used to handle custom API requests
void grabParams(AsyncWebServerRequest* request) {
    int params = request->params();
    std::string axes1;
    std::string axes2;

    for (int i = 0; i < params; i++) {
        AsyncWebParameter* param = request->getParam(i);
        if (param->name() == "Axes1") {
            axes1 = param->value().c_str();
        } else if (param->name() == "Axes2") {
            axes2 = param->value().c_str();
        }
    }
    int ax1 = atoi(axes1.c_str());
    int ax2 = atoi(axes2.c_str());
    int ax5 = ax1 - ax2;
    int ax6 = ax1 + ax2;
    Serial.printf("Axes1: %d, Axes2: %d\n", ax1, ax2);
    request->send(200, "text/plain", "OK");
}

// Note: Here are two functions that can be used to handle custom API requests
void printHelloWorld(AsyncWebServerRequest* request) {
    Serial.println("Hello World!");
    request->send(200, "text/plain", "Hello World!");
}

void blink(AsyncWebServerRequest* request) {
    digitalWrite(4, HIGH);
    Network_Utilities::my_delay(1);  // delay for 1sec
    digitalWrite(4, LOW);
    Network_Utilities::my_delay(1);  // delay for 1sec
    request->send(200, "text/plain", "Blink!");
}

//* Captive Portal Handler
class CaptiveRequestHandler : public AsyncWebHandler {
   public:
    CaptiveRequestHandler() {}
    virtual ~CaptiveRequestHandler() {}

    bool canHandle(AsyncWebServerRequest* request) {
        // request->addInterestingHeader("ANY");
        return true;
    }
    //* use SPIFFS or built in HTML wifimanager
    void handleRequest(AsyncWebServerRequest* request) {
        if (server.spiffsMounted) {
            if (server.custom_html_files.size() > 0) {
                AsyncResponseStream* response_stream =
                    request->beginResponseStream("text/html");
                for (auto& file : server.custom_html_files) {
                    if (file.endpoint == "captive_portal" ||
                        file.endpoint == "captive_portal.html" ||
                        file.endpoint == "/" ||
                        file.endpoint == "/index.html" ||
                        file.endpoint == "/index" ||
                        file.endpoint == "portal.html") {
                        response_stream->print(file.file.c_str());
                    }
                }
                request->send(response_stream);
                return;
            }
        }

        AsyncWebServerResponse* response = request->beginResponse_P(
            200, "text/html", WEB_MANAGER_HTML, WEB_MANAGER_HTML_SIZE);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    }
};

void setupServer() {
    // add command handlers to the API server
    // you can add as many as you want - you can also add methods.
    log_d("[SETUP]: Starting API Server");
    server.server.addHandler(new CaptiveRequestHandler())
        .setFilter(ON_AP_FILTER);  // only when requested from AP

    server.addAPICommand("blink", blink);
    server.addAPICommand("helloWorld", printHelloWorld);
    server.addAPICommand("params", grabParams);
    server.addAPICommand("paramsClass", [&](AsyncWebServerRequest* request) {
        Temp t;
        t.grabParams(request);
    });
    server.begin();
    log_d("[SETUP]: API Server Started");
}

void setup() {
    Serial.begin(115200);
    pinMode(4, OUTPUT);
    Serial.println("\nHello, EasyNetworkManager!");

    configHandler.config.attach(configHandler);
    //* Optionally register a custom config  - this will be saved and loaded
    configHandler.config.registerUserConfig(&customConfig);
    configHandler.config.attach(network);
    configHandler.config.attach(
        mDNS);  // attach the config manager to the mdns object - this will
                // update the config when mdns hostname changes

    configHandler.begin();  // load the config from flash

    //* Begin the network manager
    network.begin();  // setup wifi connection

    //* Begin the captive portal
    //*  Port, domain name - wildcard to capture all
    dnsServer.start(53, "*", WiFi.softAPIP());
    
    mDNS.begin();  // start mDNS service (optional)

    setupServer();  // setup the API server
    
    ota.begin();
}

void loop() {
    dnsServer.processNextRequest();
    ota.handleOTAUpdate();
}
