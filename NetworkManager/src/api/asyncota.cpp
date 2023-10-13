#include <FS.h>
#include <Update.h>
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>
#include <stdlib_noniso.h>

#include "api/Hash.h"
#include "api/asyncota.hpp"
#include "api/elegantWebpage.h"

AsyncOTA::AsyncOTA(ProjectConfig& configManager, AsyncServer_t& async_server)
    : configManager(configManager),
      async_server(async_server),
      _authRequired(false) {}

AsyncOTA::~AsyncOTA() {}

//*********************************************************************************************
//!                                     OTA Command Functions
//*********************************************************************************************

void AsyncOTA::checkAuthentication(AsyncWebServerRequest* request,
                                   const char* login, const char* password) {
    log_d("[DEBUG] Free Heap: %d", ESP.getFreeHeap());
    if (_authRequired) {
        log_d("[DEBUG] Free Heap: %d", ESP.getFreeHeap());
        log_i("Auth required");
        log_d("[DEBUG] Free Heap: %d", ESP.getFreeHeap());
        if (!request->authenticate(login, password, NULL, false)) {
            log_d("[DEBUG] Free Heap: %d", ESP.getFreeHeap());
            return request->requestAuthentication(NULL, false);
        }
    }
}

void AsyncOTA::begin() {
    // NOTE: Code adapted from:
    // https://github.com/ayushsharma82/AsyncElegantOTA/

    auto device_config = configManager.getDeviceConfig();
    auto mdns_config = configManager.getMDNSConfig();

    if (device_config.ota_password.empty()) {
        log_e(
            "Password is empty, you need to provide a password in order to "
            "setup "
            "the OTA server");
        return;
    }

    log_i("[OTA Server]: Initializing OTA Server");
    log_i(
        "[OTA Server]: Navigate to http://%s.local:81/update to update the "
        "firmware",
        mdns_config.hostname.c_str());

    log_d("[OTA Server]: Username: %s, Password: %s",
          device_config.ota_login.c_str(), device_config.ota_password.c_str());
    log_d("[DEBUG] Free Heap: %d", ESP.getFreeHeap());
    const char* login = device_config.ota_login.c_str();
    const char* password = device_config.ota_password.c_str();
    log_d("[DEBUG] Free Heap: %d", ESP.getFreeHeap());

    // Note: XHTTP_GET
    async_server.server.on(
        "/update/identity", 0b00000001, [&](AsyncWebServerRequest* request) {
            checkAuthentication(request, login, password);

            String _id = String((uint32_t)ESP.getEfuseMac(), HEX);
            _id.toUpperCase();
            request->send(200, "application/json",
                          "{\"id\": \"" + _id + "\", \"hardware\": \"ESP32\"}");
        });

    // Note: HTT_GET
    async_server.server.on(
        "/update", 0b00000001, [&](AsyncWebServerRequest* request) {
            log_d("[DEBUG] Free Heap: %d", ESP.getFreeHeap());
            checkAuthentication(request, login, password);

            if (customHandlerFunction != NULL) {
                customHandlerFunction();
            }

            AsyncWebServerResponse* response = request->beginResponse_P(
                200, "text/html", ELEGANT_HTML, ELEGANT_HTML_SIZE);
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });
    // Note: HTT_POST
    async_server.server.on(
        "/update", 0b00000010,
        [&](AsyncWebServerRequest* request) {
            checkAuthentication(request, login, password);
            // the request handler is triggered after the upload has finished...
            // create the response, add header, and send response
            AsyncWebServerResponse* response = request->beginResponse(
                (Update.hasError()) ? 500 : 200, "text/plain",
                (Update.hasError()) ? "FAIL" : "OK");
            response->addHeader("Connection", "close");
            response->addHeader("Access-Control-Allow-Origin", "*");
            request->send(response);
            configManager.save();
        },
        [&](AsyncWebServerRequest* request, String filename, size_t index,
            uint8_t* data, size_t len, bool final) {
            // Upload handler chunks in data
            checkAuthentication(request, login, password);

            if (!index) {
                if (!request->hasParam("MD5", true)) {
                    return request->send(400, "text/plain",
                                         "MD5 parameter missing");
                }

                if (!Update.setMD5(
                        request->getParam("MD5", true)->value().c_str())) {
                    return request->send(400, "text/plain",
                                         "MD5 parameter invalid");
                }
                int cmd = (filename == "filesystem") ? U_SPIFFS : U_FLASH;
                if (!Update.begin(UPDATE_SIZE_UNKNOWN,
                                  cmd)) {  // Start with max available size
                    Update.printError(Serial);
                    return request->send(400, "text/plain",
                                         "OTA could not begin");
                }
            }

            // Write chunked data to the free sketch space
            if (len) {
                if (Update.write(data, len) != len) {
                    return request->send(400, "text/plain",
                                         "OTA could not begin");
                }
            }

            if (final) {  // if the final flag is set then this is the last
                          // frame of data
                if (!Update.end(true)) {  // true to set the size to the current
                                          // progress
                    Update.printError(Serial);
                    return request->send(400, "text/plain",
                                         "Could not end OTA");
                }
            } else {
                return;
            }
        });
}
void AsyncOTA::setOTAHandler(
    AsyncOTACustomHandlerFunction customHandlerFunction) {
    this->customHandlerFunction = customHandlerFunction;
}
