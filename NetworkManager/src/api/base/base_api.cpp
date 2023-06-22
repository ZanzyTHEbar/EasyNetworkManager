#include <api/base/base_api.hpp>

BaseAPI::BaseAPI(const int CONTROL_PORT, ProjectConfig& configManager,
                 const std::string& api_url, const std::string& wifimanager_url,
                 const std::string& userCommands)
    : server(CONTROL_PORT),
      configManager(configManager),
      api_url(std::move(api_url)),
      wifimanager_url(std::move(wifimanager_url)),
      userCommands(std::move(userCommands)),
      _authRequired(false)

{}

BaseAPI::~BaseAPI() {}

void BaseAPI::begin() {
    //! i have changed this to use lambdas instead of std::bind to avoid the
    //! overhead. Lambdas are always more preferable.
    server.on("/", XHTTP_GET,
              [&](AsyncWebServerRequest* request) { request->send(200); });

#ifdef USE_WEBMANAGER
    server.on(
        wifimanager_url.c_str(), XHTTP_GET, [&](AsyncWebServerRequest* request) {
            // TODO: add authentication support
            /* if (_authRequired) {
                if (!request->authenticate(_username.c_str(),
                                           _password.c_str())) {
                    return request->requestAuthentication();
                }
            } */
            AsyncWebServerResponse* response = request->beginResponse_P(
                200, "text/html", WEB_MANAGER_HTML, WEB_MANAGER_HTML_SIZE);
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });
#endif  // USE_WEBManager

    if (initSPIFFS()) {
        if (custom_html_files.size() > 0) {
            for (auto& route : custom_html_files) {
                server.on(route.endpoint.c_str(),
                          _networkMethodsMap_inv[route.method],
                          [&](AsyncWebServerRequest* request) {
                              // TODO: add authentication support
                              /* if (_authRequired) {
                                  if (!request->authenticate(_username.c_str(),
                                                             _password.c_str()))
                              { return request->requestAuthentication();
                                  }
                              } */
                              request->send(SPIFFS, route.file.c_str(),
                                            MIMETYPE_HTML);
                          });
            }
        }
        /* server.serveStatic(wifimanager_url.c_str(), SPIFFS,
           "/wifimanager.html") .setCacheControl("max-age=600"); */
    } else {
        log_e(
            "SPIFFS not initialized - no user defined html files available. "
            "API will still function, no custom html files have been loaded. "
            "\n");
    }

    // preflight cors check
    server.on("/", XHTTP_OPTIONS, [&](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse(204);
        response->addHeader("Access-Control-Allow-Methods",
                            "PUT,POST,GET,OPTIONS");
        response->addHeader("Access-Control-Allow-Headers",
                            "Accept, Content-Type, Authorization, FileSize");
        response->addHeader("Access-Control-Allow-Credentials", "true");
        request->send(response);
    });

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

    server.onNotFound(
        [&](AsyncWebServerRequest* request) { notFound(request); });
}

void BaseAPI::notFound(AsyncWebServerRequest* request) const {
    if (_networkMethodsMap.find(request->method()) !=
        _networkMethodsMap.end()) {
        log_i("%s: http://%s%s/\n",
              _networkMethodsMap.at(request->method()).c_str(),
              request->host().c_str(), request->url().c_str());
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "Request %s Not found: %s",
                 _networkMethodsMap.at(request->method()).c_str(),
                 request->url().c_str());
        request->send(404, "text/plain", buffer);
    } else {
        request->send(404, "text/plain",
                      "Request Not found using unknown method");
    }
}

//*********************************************************************************************
//!                                     Command Functions
//*********************************************************************************************
void BaseAPI::setWiFi(AsyncWebServerRequest* request) {
    switch (_networkMethodsMap_enum[request->method()]) {
        case POST: {
            std::string networkName;
            std::string ssid;
            std::string password;
            std::string ota_password;
            std::string mdns;
            int ota_port = 0;
            uint8_t channel = 0;
            uint8_t power = 0;
            uint8_t adhoc = 0;

            int params = request->params();
            log_d("Number of Params: %d", params);
            for (int i = 0; i < params; i++) {
                AsyncWebParameter* param = request->getParam(i);
                if (param->name() == "networkName") {
                    networkName.assign(param->value().c_str());
                } else if (param->name() == "ssid") {
                    ssid.assign(param->value().c_str());
                } else if (param->name() == "password") {
                    password.assign(param->value().c_str());
                } else if (param->name() == "channel") {
                    channel = (uint8_t)atoi(param->value().c_str());
                } else if (param->name() == "power") {
                    power = (uint8_t)atoi(param->value().c_str());
                } else if (param->name() == "ota_password") {
                    ota_password.assign(param->value().c_str());
                } else if (param->name() == "ota_port") {
                    ota_port = atoi(param->value().c_str());
                } else if (param->name() == "mdns") {
                    mdns.assign(param->value().c_str());
                } else if (param->name() == "adhoc") {
                    adhoc = (uint8_t)atoi(param->value().c_str());
                }
                log_i("%s[%s]: %s\n",
                      _networkMethodsMap[request->method()].c_str(),
                      param->name().c_str(), param->value().c_str());
            }
            // note: We're passing empty params by design, this is done to reset
            // specific fields
            configManager.setWifiConfig(networkName, ssid, password, channel,
                                        power, adhoc, true);

            if (!mdns.empty()) {
                if (!configManager.setMDNSConfig(mdns, true)) {
                    request->send(400, MIMETYPE_JSON,
                                  "{\"msg\":\"Error. MDNS Name is not a "
                                  "valid alpha numeric string.\"}");
                }
                log_i("MDNS Name set to: %s", mdns.c_str());
            }

            if (!ota_password.empty()) {
                configManager.setDeviceConfig(ota_password, ota_port, true);
                log_i("OTA Password set to: %s", ota_password.c_str());
            }
            this->save(request);
            break;
        }
        case DELETE: {
            configManager.deleteWifiConfig(request->arg("networkName").c_str(),
                                           true);
            request->send(200, MIMETYPE_JSON,
                          "{\"msg\":\"Done. Wifi Creds have been deleted.\"}");
            break;
        }
        default: {
            request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Request\"}");
            request->redirect("/");
            break;
        }
    }
}

void BaseAPI::setWiFiTXPower(AsyncWebServerRequest* request) {
    switch (_networkMethodsMap_enum[request->method()]) {
        case GET: {
            request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Request\"}");
            break;
        }
        case POST: {
            int params = request->params();

            uint8_t txPower = 0;

            for (int i = 0; i < params; i++) {
                AsyncWebParameter* param = request->getParam(i);
                if (param->name() == "txPower") {
                    txPower = atoi(param->value().c_str());
                }
            }
            configManager.setWiFiTxPower(txPower, true);
            configManager.wifiTxPowerConfigSave();
            request->send(200, MIMETYPE_JSON,
                          "{\"msg\":\"Done. TX Power has been set.\"}");
        }
    }
}

void BaseAPI::handleJson(AsyncWebServerRequest* request) {
    switch (_networkMethodsMap_enum[request->method()]) {
        case GET: {
            request->send(200, MIMETYPE_JSON,
                          configManager.getDeviceDataJson().deviceJson.c_str());
            break;
        }
        case POST: {
            if (request->hasParam("json", true)) {
                AsyncWebParameter* param = request->getParam("json", true);
                log_i("%s[%s]: %s\n",
                      _networkMethodsMap[request->method()].c_str(),
                      param->name().c_str(), param->value().c_str());
                // configManager.setJsonConfig(param->value().c_str());
                request->send(200, MIMETYPE_JSON,
                              "{\"msg\":\"Done. Config has been set.\"}");
            } else {
                request->send(400, MIMETYPE_JSON,
                              "{\"msg\":\"Invalid Request\"}");
            }
            break;
        }
        default: {
            request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Request\"}");
            request->redirect("/");
            break;
        }
    }
}

void BaseAPI::getJsonConfig(AsyncWebServerRequest* request) {
    // returns the current stored config in case it get's deleted on the PC.
    switch (_networkMethodsMap_enum[request->method()]) {
        case GET: {
            std::string wifiConfigSerialized = "\"wifi_config\": [";
            auto networksConfigs = configManager.getWifiConfigs();
            for (auto& networkConfig : networksConfigs) {
                wifiConfigSerialized += networkConfig.toRepresentation();

                if (&networkConfig != &networksConfigs.back())
                    wifiConfigSerialized += ",";
            }
            wifiConfigSerialized += "]";

            std::string json = Helpers::format_string(
                "{%s, %s, %s, %s, %s}",
                configManager.getDeviceConfig().toRepresentation().c_str(),
                configManager.getWifiTxPowerConfig().toRepresentation().c_str(),
                wifiConfigSerialized.c_str(),
                configManager.getMDNSConfig().toRepresentation().c_str(),
                configManager.getAPWifiConfig().toRepresentation().c_str());
            request->send(200, MIMETYPE_JSON, json.c_str());
            break;
        }
        default: {
            request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Request\"}");
            break;
        }
    }
}

void BaseAPI::rebootDevice(AsyncWebServerRequest* request) {
    switch (_networkMethodsMap_enum[request->method()]) {
        case GET: {
            request->send(200, MIMETYPE_JSON, "{\"msg\":\"Rebooting Device\"}");
            ESP.restart();
        }
        default: {
            request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Request\"}");
            break;
        }
    }
}

void BaseAPI::factoryReset(AsyncWebServerRequest* request) {
    switch (_networkMethodsMap_enum[request->method()]) {
        case GET: {
            log_d("Factory Reset");
            bool success = configManager.reset();
            char buf[100];
            snprintf(buf, sizeof(buf), "{\"msg\":\"Factory Reset - %s\"}",
                     success ? "Done" : "Failed");
            request->send(200, MIMETYPE_JSON, buf);
        }
        default: {
            request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Request\"}");
            break;
        }
    }
}

/**
 * @brief Remove a command handler from the API
 *
 * @param request
 * @return \c void
 */
void BaseAPI::removeRoute(AsyncWebServerRequest* request) {
    log_i("Request: %s", request->url().c_str());
    int params = request->params();
    auto it_map = route_map.find(request->pathArg(0).c_str());
    log_i("Request: %s", request->pathArg(0).c_str());

    auto it = it_map->second.find(request->pathArg(1).c_str());
    if (it != it_map->second.end()) {
        switch (_networkMethodsMap_enum[request->method()]) {
            case DELETE: {
                it_map->second.erase(it);
                request->send(200, MIMETYPE_JSON,
                              "{\"msg\":\"Route Removed\"}");
                break;
            }
            default: {
                request->send(400, MIMETYPE_JSON,
                              "{\"msg\":\"Invalid Request\"}");
                break;
            }
        }
    } else {
        request->send(400, MIMETYPE_JSON, "{\"msg\":\"Route Not Found\"}");
    }
}

void BaseAPI::ping(AsyncWebServerRequest* request) {
    request->send(200, MIMETYPE_JSON, "{\"msg\": \"ok\" }");
}

void BaseAPI::save(AsyncWebServerRequest* request) {
    configManager.save();
    request->send(200, MIMETYPE_JSON, "{\"msg\": \"ok\" }");
}

void BaseAPI::rssi(AsyncWebServerRequest* request) {
    int rssi = Network_Utilities::getStrength(
        request->getParam("points")->value().toInt());
    char _rssiBuffer[20];
    snprintf(_rssiBuffer, sizeof(_rssiBuffer), "{\"rssi\": %d }", rssi);
    request->send(200, MIMETYPE_JSON, _rssiBuffer);
}

#ifdef USE_ASYNCOTA
//*********************************************************************************************
//!                                     OTA Command Functions
//*********************************************************************************************

void BaseAPI::checkAuthentication(AsyncWebServerRequest* request,
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

void BaseAPI::beginOTA() {
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
    server.on(
        "/update/identity", 0b00000001, [&](AsyncWebServerRequest* request) {
            checkAuthentication(request, login, password);

            String _id = String((uint32_t)ESP.getEfuseMac(), HEX);
            _id.toUpperCase();
            request->send(200, "application/json",
                          "{\"id\": \"" + _id + "\", \"hardware\": \"ESP32\"}");
        });

    // Note: HTT_GET
    server.on("/update", 0b00000001, [&](AsyncWebServerRequest* request) {
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
    server.on(
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
            this->save(request);
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
void BaseAPI::setOTAHandler(
    AsyncOTACustomHandlerFunction customHandlerFunction) {
    this->customHandlerFunction = customHandlerFunction;
}

#endif  // USE_ASYNCOTA