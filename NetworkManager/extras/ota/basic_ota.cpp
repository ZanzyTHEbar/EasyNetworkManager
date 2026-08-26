// Standalone basic OTA sketch, compiled by [env:esp32dev_extras_ota].
// ponytail: config-store password rotation dropped for the standalone demo;
// wire ProjectConfig back in if this grows beyond extras/.
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>

#ifndef OTA_SERVER_PORT
#define OTA_SERVER_PORT 3232
#endif

#ifndef OTA_PASSWORD
#define OTA_PASSWORD ""
#endif

#ifndef MDNS_HOSTNAME
#define MDNS_HOSTNAME "easynetworkmanager-ota"
#endif

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

namespace {
unsigned long _bootTimestamp = 0;
bool _isOtaEnabled = false;

// OTA is live for 5 minutes after boot, then disabled until restart.
constexpr unsigned long kOtaWindowMs = 60000UL * 5;
}  // namespace

void setup() {
    Serial.begin(115200);

#if !defined(EASYNETWORKMANAGER_ALLOW_INSECURE_OTA)
    // Development-only: the current basic OTA stack is plain HTTP. This guard
    // mirrors the fail-closed behavior of the library's async OTA path.
    Serial.println(
        "[Basic OTA]: disabled; enable EASYNETWORKMANAGER_ALLOW_INSECURE_OTA "
        "only for trusted development networks until signed OTA is configured");
    return;
#else
    if (strlen(OTA_PASSWORD) == 0) {
        Serial.println("[Basic OTA]: THE OTA PASSWORD IS REQUIRED, [[ABORTING]]");
        return;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("[Basic OTA]: Connecting to Wi-Fi");
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[Basic OTA]: Wi-Fi connect failed, [[ABORTING]]");
        return;
    }

    ArduinoOTA.setPort(OTA_SERVER_PORT);
    ArduinoOTA.setPassword(OTA_PASSWORD);

    ArduinoOTA
        .onStart([]() {
            String type =
                (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
            Serial.printf("[Basic OTA]: Start updating %s\n", type.c_str());
        })
        .onEnd([]() {
            Serial.println("\n[Basic OTA]: OTA updated finished successfully!");
        })
        .onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("[Basic OTA]: Progress: %u%%\r",
                          (progress / (total / 100)));
        })
        .onError([](ota_error_t error) {
            Serial.printf("Error[%u]: ", error);
            switch (error) {
                case OTA_AUTH_ERROR:
                    Serial.println("[Basic OTA]: Auth Failed");
                    break;
                case OTA_BEGIN_ERROR:
                    Serial.println("[Basic OTA]: Begin Failed");
                    break;
                case OTA_CONNECT_ERROR:
                    Serial.println("[Basic OTA]: Connect Failed");
                    break;
                case OTA_RECEIVE_ERROR:
                    Serial.println("[Basic OTA]: Receive Failed");
                    break;
                case OTA_END_ERROR:
                    Serial.println("[Basic OTA]: End Failed");
                    break;
            }
        });

    Serial.println("[Basic OTA]: Starting up basic OTA server");
    Serial.println(
        "[Basic OTA]: OTA will be live for 5mins, after which it will be "
        "disabled until restart");
    ArduinoOTA.setHostname(MDNS_HOSTNAME);
    ArduinoOTA.begin();
    _bootTimestamp = millis();
    _isOtaEnabled = true;
#endif
}

void loop() {
    if (!_isOtaEnabled) {
        return;
    }
    if (millis() - _bootTimestamp >= kOtaWindowMs) {
        // we're disabling ota after first 5 minutes so that nothing bad
        // happens during runtime
        _isOtaEnabled = false;
        Serial.println("[Basic OTA]: From now on, OTA is disabled");
        return;
    }
    ArduinoOTA.handle();
}
