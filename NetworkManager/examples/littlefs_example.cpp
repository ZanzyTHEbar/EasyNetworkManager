#include <Arduino.h>
#include <LittleFS.h>
#include <EasyNetworkManager.h>

/**
 * @brief Example setup for LittleFS with EasyNetworkManager
 * This example demonstrates how to mount and use LittleFS alongside
 * the EasyNetworkManager.
 */

// Define filesystem path for LittleFS
const char* FS_PATH = "/littlefs";

EasyNetworkManager networkManager("easynetwork", MDNS_HOSTNAME, WIFI_SSID,
                                  WIFI_PASSWORD, 1, "_easynetwork", "test",
                                  "_tcp", "_api_port", "80", true, false);

AsyncServer_t async_server(80, networkManager.configHandler->config, "/api",
                           "/wifimanager", "/mycommands", "/json");

APIServer api(networkManager.configHandler->config, async_server);

void setupFilesystem() {
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
        return;
    }
    Serial.println("LittleFS Mount Successful");

    // Example: Create a file
    File file = LittleFS.open("/test.txt", FILE_WRITE);
    if (file) {
        file.println("Hello from LittleFS!");
        file.close();
        Serial.println("File written successfully");
    }

    // Example: Read a file
    if (LittleFS.exists("/test.txt")) {
        file = LittleFS.open("/test.txt", FILE_READ);
        if (file) {
            String content = file.readString();
            Serial.print("File content: ");
            Serial.println(content);
            file.close();
        }
    }
}

void setupServer() {
    setupFilesystem();
    api.begin();
    log_d("[SETUP]: API Server Started");
}

void setup() {
    Serial.begin(115200);
    Serial.println("\nHello, EasyNetworkManager with LittleFS!");

    setupServer();
}

void loop() {
    // Your loop code here
    delay(1000);
}
