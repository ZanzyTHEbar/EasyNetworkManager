#include <Arduino.h>
#include <EasyNetworkManager.h>
//! Only needed for BLOCKING OTA. AsyncOTA is recommended and builtin
// #include <network/ota/basic_ota.hpp>

/**
 * @brief Setup the EasyNetworkManager Instance
 * @note The EasyNetworkManager constructor takes 12 parameters:
 * @param config_name The name of the project (used to create the config
 * file name)
 * @param hostname The hostname for your device on the network(used for
 * mDNS, OTA, etc.)
 * @param ssid The SSID of the WiFi network to connect to
 * @param password The password of the WiFi network to connect to
 * @param channel The channel of the WiFi network to connect to
 * @param service_name The name of the service
 * @param service_instance_name The instance name of the service
 * @param service_protocol The protocol of the service
 * @param service_description The description of the service
 * @param service_port The port of the service
 * @param enable_mdns Enable mDNS
 * @param enable_adhoc Enable Adhoc
 */
EasyNetworkManager networkManager("easynetwork", MDNS_HOSTNAME, WIFI_SSID,
                                  WIFI_PASSWORD, 1, "_easynetwork", "test",
                                  "_tcp", "_api_port", "80", true, false);

/**
 * @brief Setup the AsyncServer Instance
 * @note The AsyncServer constructor takes 5 parameters:
 * @param port The port to listen on
 * @param config The config manager
 * @param api_path The path to the API
 * @param wifi_manager_path The path to the WiFi Manager
 * @param command_path The path to the command handler
 */
AsyncServer_t async_server(80, networkManager.configHandler->config, "/api",
                           "/wifimanager", "/mycommands");

/**
 * @brief Setup the API Server Instance
 * @note The API Server constructor takes 2 parameters:
 * @param config The config manager
 * @param server The AsyncServer instance
 */
APIServer api(networkManager.configHandler->config, async_server);

// Note: Not required if you are going to use the AsyncOTA feature
// OTA ota(networkManager.configHandler->config);

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

void setupServer() {
    // This is an example of how to add a custom html file to the web server
    // Note: The first parameter is the url endpoint to access the file
    // Note: The second parameter is the path to the file on the SPIFFS
    // Note: The third parameter is the HTTP method to use to access the file
    async_server.custom_html_files.emplace_back("/hello", "/helloWorld.html",
                                                "GET");
    async_server.custom_html_files.emplace_back("/goodbye", "/goodbye.html",
                                                "POST");

    api.addAPICommand("blink", blink);
    api.addAPICommand("helloWorld", printHelloWorld);
    api.addAPICommand("params", grabParams);
    api.addAPICommand("paramsClass", [&](AsyncWebServerRequest* request) {
        Temp t;
        t.grabParams(request);
    });
    api.begin();
    log_d("[SETUP]: API Server Started");
}

void setup() {
    Serial.begin(115200);
    pinMode(4, OUTPUT);
    Serial.println("\nHello, EasyNetworkManager!");

    //* Optionally register a custom config  - this will be saved and loaded
    networkManager.configHandler->config.registerUserConfig(&customConfig);
    networkManager.begin();

    /**
     * @brief This Function is used to handle the state changes of the WiFi
     */
    updateWrapper<WiFiState_e>(
        networkManager.configHandler->config.getState(
            networkManager.wifiHandler->getName()),
        [](WiFiState_e state) {
            switch (state) {
                //! intentional fallthrough case
                case WiFiState_e::WiFiState_ADHOC:
                case WiFiState_e::WiFiState_Connected: {
                    setupServer();
                    break;
                }
                case WiFiState_e::WiFiState_Disconnected: {
                    break;
                }
                case WiFiState_e::WiFiState_Disconnecting: {
                    break;
                }
                case WiFiState_e::WiFiState_Connecting: {
                    break;
                }
                case WiFiState_e::WiFiState_Error: {
                    break;
                }
            }
        });
    // ota.begin();
}

void loop() {
    // ota.handleOTAUpdate();
}
