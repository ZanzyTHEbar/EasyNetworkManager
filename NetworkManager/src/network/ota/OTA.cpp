#include <network/ota/OTA.hpp>

OTA::OTA(ProjectConfig* deviceConfig)
    : _deviceConfig(deviceConfig), _bootTimestamp(0), _isOtaEnabled(true) {}

OTA::~OTA() {}

void OTA::begin() {
    log_i("Setting up OTA updates");
    auto localConfig = _deviceConfig->getDeviceConfig();
    auto mdnsConfig = _deviceConfig->getMDNSConfig();
    if (localConfig->OTAPassword.empty()) {
        log_e("THE OTA PASSWORD IS REQUIRED, [[ABORTING]]");
        return;
    }

    ArduinoOTA.setPort(localConfig->OTAPort);

    ArduinoOTA
        .onStart([]() {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
                type = "sketch";
            else  // U_SPIFFS
                type = "filesystem";
        })
        .onEnd([]() { Serial.println("OTA updated finished successfully!"); })
        .onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        })
        .onError([](ota_error_t error) {
            log_e("Error[%u]: ", error);
            switch (error) {
                case OTA_AUTH_ERROR:
                    log_e("Auth Failed");
                    break;
                case OTA_BEGIN_ERROR:
                    log_e("Begin Failed");
                    break;
                case OTA_CONNECT_ERROR:
                    log_e("Connect Failed");
                    break;
                case OTA_RECEIVE_ERROR:
                    log_e("Receive Failed");
                    break;
                case OTA_END_ERROR:
                    log_e("End Failed");
                    break;
            }
        });

    Serial.println("Starting up basic OTA server");
    Serial.println(
        "OTA will be live for 5mins, after which it will be disabled until "
        "restart");
    ArduinoOTA.setHostname(mdnsConfig->hostname.c_str());
    ArduinoOTA.begin();
    _bootTimestamp = millis();
}

void OTA::handleOTAUpdate() {
    if (_isOtaEnabled) {
        if (_bootTimestamp + (60000 * 5) < millis()) {
            // we're disabling ota after first 5 minutes so that nothing bad
            // happens during runtime
            _isOtaEnabled = false;
            Serial.println("From now on, OTA is disabled");
            return;
        }
        ArduinoOTA.handle();
    }
}