#include <api/base_api.hpp>

// TODO: Implement proper JSON post and get
//?
// https://github.com/me-no-dev/ESPAsyncWebServer/tree/master#arduinojson-advanced-response
// TODO: Implement authentication
// TODO: Implement proper visitor pattern for JSON serialization and
// deserialization

BaseAPI::BaseAPI(ProjectConfig& configManager) : configManager(configManager) {}

BaseAPI::~BaseAPI() {}

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
