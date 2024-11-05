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

/**
 * @brief Setup the API Server Instance
 * @note The API Server constructor takes 2 parameters:
 * @param config The config manager
 * @param server The AsyncServer instance
 */
APIServer api(networkManager.configHandler->config, async_server);

void setupServer() {
    // This is an example of how to add a custom html file to the web server
    // Note: The first parameter is the url endpoint to access the file
    // Note: The second parameter is the path to the file on the SPIFFS
    // Note: The third parameter is the HTTP method to use to access the file
    async_server.custom_html_files.emplace_back("/hello", "/helloWorld.html",
                                                "GET");
    async_server.custom_html_files.emplace_back("/goodbye", "/goodbye.html",
                                                "POST");

    api.begin();
    log_d("[SETUP]: API Server Started");
}

void setup() {
    Serial.begin(115200);
    pinMode(4, OUTPUT);
    Serial.println("\nHello, EasyNetworkManager!");
    networkManager.begin();

    setupServer();
}

void loop() {}
