#include <Arduino.h>

#ifdef WOKWI_FIXTURE
#include <EasyNetworkManager.h>

#ifndef WOKWI_API_LOGIN
#error "WOKWI_API_LOGIN must be configured for the Wokwi fixture"
#endif
#ifndef WOKWI_API_PASSWORD
#error "WOKWI_API_PASSWORD must be configured for the Wokwi fixture"
#endif

EasyNetworkManager networkManager("easynetwork", MDNS_HOSTNAME, WIFI_SSID,
                                  WIFI_PASSWORD, 1, "_easynetwork", "test",
                                  "_tcp", "_api_port", "80", true, false);

AsyncServer_t asyncServer(80, networkManager.configHandler->config, "/api",
                          "/wifimanager", "/mycommands", "/json");
AsyncOTA asyncOta(networkManager.configHandler->config, asyncServer);
APIServer api(networkManager.configHandler->config, asyncServer, &asyncOta);

void setup() {
    Serial.begin(115200);
    Serial.println("[WOKWI] startup");

    networkManager.begin();
    Serial.println("[WOKWI] network ready");

    auto& deviceConfig = networkManager.configHandler->config.getDeviceConfig();
    deviceConfig.ota_login = WOKWI_API_LOGIN;
    deviceConfig.ota_password = WOKWI_API_PASSWORD;

    api.begin();
    Serial.println(
        "[WOKWI] ready: /api /wifimanager /mycommands /json /update");
}

void loop() { delay(1); }
#else
void setup() {}

void loop() {}
#endif
