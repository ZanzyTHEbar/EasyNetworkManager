#include <Arduino.h>
#include <EasyNetworkManager.h>

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
                           "/wifimanager", "/mycommands", "/json");

/**
 * @brief Setup the API Server Instance
 * @note The API Server constructor takes 2 parameters:
 * @param config The config manager
 * @param server The AsyncServer instance
 */
APIServer api(networkManager.configHandler->config, async_server);

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

void setupServer() {
    // add command handlers to the API server
    // you can add as many as you want - you can also add methods.
    log_d("[SETUP]: Starting API Server");
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
    networkManager.begin();

    /**
     * @brief This Function is used to handle the state changes of the WiFi
     */
    updateWrapper<WiFiState_e>(
        networkManager.configHandler->config.getState(
            networkManager.wifiHandler->getID()),
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
}

void loop() {}
