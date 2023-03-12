#include <api/baseAPI/baseAPI.hpp>

BaseAPI::BaseAPI(int CONTROL_PORT, ProjectConfig* configManager,
                 const std::string& api_url, const std::string& wifimanager_url,
                 const std::string& userCommands)
    : server(CONTROL_PORT),
      configManager(configManager),
      api_url(std::move(api_url)),
      wifimanager_url(std::move(wifimanager_url)),
      userCommands(std::move(userCommands))

{}

BaseAPI::~BaseAPI() {}

void BaseAPI::begin() {
    //! i have changed this to use lambdas instead of std::bind to avoid the
    //! overhead. Lambdas are always more preferable.
    server.on("/", HTTP_GET,
              [&](AsyncWebServerRequest* request) { request->send(200); });

#ifdef USE_WEBMANAGER
    server.on(
        wifimanager_url.c_str(), HTTP_GET, [&](AsyncWebServerRequest* request) {
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
        if (userEndpointsVector.size() > 0) {
            for (auto& route : userEndpointsVector) {
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
    server.on("/", HTTP_OPTIONS, [&](AsyncWebServerRequest* request) {
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
            configManager->setWifiConfig(networkName, ssid, password, channel,
                                         power, adhoc, true);

            if (!mdns.empty()) {
                if (!configManager->setMDNSConfig(mdns, true)) {
                    request->send(400, MIMETYPE_JSON,
                                  "{\"msg\":\"Error. MDNS Name is not a "
                                  "valid alpha numeric string.\"}");
                }
                log_i("MDNS Name set to: %s", mdns.c_str());
            }

            if (!ota_password.empty()) {
                configManager->setDeviceConfig(ota_password, ota_port, true);
                log_i("OTA Password set to: %s", ota_password.c_str());
            }

            request->send(
                200, MIMETYPE_JSON,
                "{\"msg\":\"Done. Network Credentials have been set. Rebooting\"}");
            this->save(request);
            break;
        }
        case DELETE: {
            configManager->deleteWifiConfig(request->arg("networkName").c_str(),
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
            configManager->setWiFiTxPower(txPower, true);
            configManager->wifiTxPowerConfigSave();
            request->send(200, MIMETYPE_JSON,
                          "{\"msg\":\"Done. TX Power has been set.\"}");
        }
    }
}

void BaseAPI::handleJson(AsyncWebServerRequest* request) {
    switch (_networkMethodsMap_enum[request->method()]) {
        case GET: {
            request->send(
                200, MIMETYPE_JSON,
                configManager->getDeviceDataJson()->deviceJson.c_str());
            break;
        }
        case POST: {
            if (request->hasParam("json", true)) {
                AsyncWebParameter* param = request->getParam("json", true);
                log_i("%s[%s]: %s\n",
                      _networkMethodsMap[request->method()].c_str(),
                      param->name().c_str(), param->value().c_str());
                // configManager->setJsonConfig(param->value().c_str());
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
            auto networksConfigs = configManager->getWifiConfigs();
            for (auto networkIterator = networksConfigs->begin();
                 networkIterator != networksConfigs->end(); networkIterator++) {
                wifiConfigSerialized +=
                    networkIterator->toRepresentation() +
                    (std::next(networkIterator) != networksConfigs->end() ? ","
                                                                          : "");
            }
            wifiConfigSerialized += "]";

            std::string json = Helpers::format_string(
                "{%s, %s, %s, %s, %s}",
                configManager->getDeviceConfig()->toRepresentation().c_str(),
                configManager->getWifiTxPowerConfig()
                    ->toRepresentation()
                    .c_str(),
                wifiConfigSerialized.c_str(),
                configManager->getMDNSConfig()->toRepresentation().c_str(),
                configManager->getAPWifiConfig()->toRepresentation().c_str());
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
            bool success = configManager->reset();
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
    configManager->save();
    request->send(200, MIMETYPE_JSON, "{\"msg\": \"ok\" }");
}

void BaseAPI::rssi(AsyncWebServerRequest* request) {
    int rssi = Network_Utilities::getStrength(
        request->getParam("points")->value().toInt());
    char _rssiBuffer[20];
    snprintf(_rssiBuffer, sizeof(_rssiBuffer), "{\"rssi\": %d }", rssi);
    request->send(200, MIMETYPE_JSON, _rssiBuffer);
}