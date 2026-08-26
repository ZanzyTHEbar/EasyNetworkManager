#include <api/base_api.hpp>

#include <algorithm>
#include <cerrno>
#include <cstdlib>

namespace {

bool parseInteger(const String& value, int minimum, int maximum, int& result) {
    if (value.isEmpty()) {
        return false;
    }

    char* end = nullptr;
    errno = 0;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (errno != 0 || end == value.c_str() || *end != '\0' ||
        parsed < minimum || parsed > maximum) {
        return false;
    }

    result = static_cast<int>(parsed);
    return true;
}

}  // namespace

// https://github.com/me-no-dev/ESPAsyncWebServer/tree/master#arduinojson-advanced-response

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
            std::string ota_login;
            std::string ota_password;
            std::string mdns;
            int ota_port = 0;
            uint8_t channel = 1;
            uint8_t power = configManager.getWifiTxPowerConfig().power;
            uint8_t adhoc = 0;
            bool invalidNumericParameter = false;

            int params = request->params();
            log_d("Number of Params: %d", params);
            for (int i = 0; i < params; i++) {
                const AsyncWebParameter* param = request->getParam(i);
                if (param->name() == "networkName") {
                    networkName.assign(param->value().c_str());
                } else if (param->name() == "ssid") {
                    ssid.assign(param->value().c_str());
                } else if (param->name() == "password") {
                    password.assign(param->value().c_str());
                } else if (param->name() == "channel") {
                    int parsed = 0;
                    invalidNumericParameter =
                        invalidNumericParameter ||
                        !parseInteger(param->value(), 1, 14, parsed);
                    channel = static_cast<uint8_t>(parsed);
                } else if (param->name() == "power") {
                    int parsed = 0;
                    invalidNumericParameter =
                        invalidNumericParameter ||
                        !parseInteger(param->value(), 0, 78, parsed);
                    power = static_cast<uint8_t>(parsed);
                } else if (param->name() == "ota_password") {
                    ota_password.assign(param->value().c_str());
                } else if (param->name() == "ota_login") {
                    ota_login.assign(param->value().c_str());
                } else if (param->name() == "ota_port") {
                    invalidNumericParameter =
                        invalidNumericParameter ||
                        !parseInteger(param->value(), 1, 65535, ota_port);
                } else if (param->name() == "mdns") {
                    mdns.assign(param->value().c_str());
                } else if (param->name() == "adhoc") {
                    int parsed = 0;
                    invalidNumericParameter =
                        invalidNumericParameter ||
                        !parseInteger(param->value(), 0, 1, parsed);
                    adhoc = static_cast<uint8_t>(parsed);
                }
                const bool is_secret = param->name() == "password" ||
                                       param->name() == "ota_password";
                log_i("%s[%s]: %s\n",
                      _networkMethodsMap[request->method()].c_str(),
                      param->name().c_str(),
                      is_secret ? "<redacted>" : param->value().c_str());
            }
            if (networkName.empty()) {
                networkName = ssid;
            }
            if (ssid.empty() || networkName.empty() || invalidNumericParameter) {
                request->send(400, MIMETYPE_JSON,
                              "{\"msg\":\"Invalid WiFi parameters\"}");
                return;
            }

            if (!mdns.empty() && !configManager.setMDNSConfig(mdns, false)) {
                request->send(400, MIMETYPE_JSON,
                              "{\"msg\":\"Error. MDNS Name is not a "
                              "valid alpha numeric string.\"}");
                return;
            }

            // Reject a new network before mutating anything when the saved
            // network list is full (mirrors the /provision capacity check).
            const auto& networks = configManager.getWifiConfigs();
            const bool updatesExisting =
                std::any_of(networks.begin(), networks.end(),
                            [&ssid](const auto& network) {
                                return network.ssid == ssid;
                            });
            if (!updatesExisting && networks.size() >= 3) {
                request->send(
                    400, MIMETYPE_JSON,
                    "{\"msg\":\"No network configuration capacity\"}");
                return;
            }

            configManager.setWifiConfig(networkName, ssid, password, channel,
                                        power, adhoc, false);

            if (!ota_login.empty() || !ota_password.empty() || ota_port > 0) {
                const auto& current = configManager.getDeviceConfig();
                configManager.setDeviceConfig(
                    ota_login.empty() ? current.ota_login : ota_login,
                    ota_password.empty() ? current.ota_password : ota_password,
                    ota_port > 0 ? ota_port : current.ota_port, true);
                log_i("OTA credentials updated");
            }

            configManager.save();
            if (!configManager.lastSaveSucceeded()) {
                request->send(500, MIMETYPE_JSON,
                              "{\"msg\":\"Configuration save failed\"}");
                return;
            }
            if (!mdns.empty()) {
                configManager.notifyAll(Event_e::mdnsConfigUpdated);
                log_i("MDNS Name set to: %s", mdns.c_str());
            }
            request->send(200, MIMETYPE_JSON, "{\"msg\": \"ok\" }");
            break;
        }
        case DELETE: {
            if (!request->hasParam("networkName")) {
                request->send(400, MIMETYPE_JSON,
                              "{\"msg\":\"networkName is required\"}");
                return;
            }
            configManager.deleteWifiConfig(request->arg("networkName").c_str(),
                                           true);
            configManager.save();
            if (!configManager.lastSaveSucceeded()) {
                request->send(500, MIMETYPE_JSON,
                              "{\"msg\":\"Configuration save failed\"}");
                return;
            }
            request->send(200, MIMETYPE_JSON,
                          "{\"msg\":\"Done. Wifi Creds have been deleted.\"}");
            break;
        }
        default: {
            request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Request\"}");
            return;
        }
    }
}

void BaseAPI::setWiFiTXPower(AsyncWebServerRequest* request) {
    switch (_networkMethodsMap_enum[request->method()]) {
        case POST: {
            int parsedPower = 0;
            bool txPowerFound = false;
            bool invalidTxPower = false;

            const int params = request->params();
            log_d("Number of Params: %d", params);
            for (int i = 0; i < params; i++) {
                const AsyncWebParameter* param = request->getParam(i);
                if (param->name() == "txPower") {
                    txPowerFound = true;
                    invalidTxPower =
                        !parseInteger(param->value(), 0, 78, parsedPower);
                }
            }
            if (!txPowerFound || invalidTxPower) {
                request->send(400, MIMETYPE_JSON,
                              "{\"msg\":\"Invalid TX Power parameter\"}");
                return;
            }
            configManager.setWiFiTxPower(static_cast<uint8_t>(parsedPower),
                                         true);
            configManager.wifiTxPowerConfigSave();
            if (!configManager.lastSaveSucceeded()) {
                request->send(500, MIMETYPE_JSON,
                               "{\"msg\":\"Configuration save failed\"}");
                return;
            }
            request->send(200, MIMETYPE_JSON,
                          "{\"msg\":\"Done. TX Power has been set.\"}");
            break;
        }
        default: {
            request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Request\"}");
            break;
        }
    }
}

void BaseAPI::handleJson(AsyncWebServerRequest* request, JsonVariant& jsonData) {
    switch (_networkMethodsMap_enum[request->method()]) {
        case POST: {
            auto jsonObj = jsonData.as<JsonObject>();
            std::string json;
            serializeJson(jsonObj, json);
            configManager.setDeviceDataJson(json, true);
            configManager.save();
            if (!configManager.lastSaveSucceeded()) {
                request->send(500, MIMETYPE_JSON,
                              "{\"msg\":\"Configuration save failed\"}");
                return;
            }
            request->send(200, MIMETYPE_JSON,
                          "{\"msg\":\"Done. Device Data "
                          "has been set.\"}");
            break;
        }
        default: {
            request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Request\"}");
            return;
        }
    }
}

void BaseAPI::getDeviceConfigData(AsyncWebServerRequest* request) {
    switch (_networkMethodsMap_enum[request->method()]) {
        case GET: {
            AsyncJsonResponse* response = new AsyncJsonResponse();
            response->addHeader("EasyNetworkManager", "1.0");
            auto root = response->getRoot();
            root["deviceData"] =
                configManager.getDeviceDataJson().deviceJson.c_str();
            response->setLength();
            request->send(response);
            break;
        }
        default: {
            request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Request\"}");
            return;
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
            return;
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
            return;
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
    auto it_map = route_map.find(request->pathArg(0).c_str());
    log_i("Request: %s", request->pathArg(0).c_str());

    if (it_map == route_map.end()) {
        request->send(400, MIMETYPE_JSON, "{\"msg\":\"Route Not Found\"}");
        return;
    }

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
    if (!configManager.lastSaveSucceeded()) {
        request->send(500, MIMETYPE_JSON,
                      "{\"msg\":\"Configuration save failed\"}");
        return;
    }
    request->send(200, MIMETYPE_JSON, "{\"msg\": \"ok\" }");
}

void BaseAPI::rssi(AsyncWebServerRequest* request) {
    // Single-shot reading: multi-point averaging sleeps between samples and
    // must never run on the async_tcp task.
    (void)request;
    char _rssiBuffer[20];
    snprintf(_rssiBuffer, sizeof(_rssiBuffer), "{\"rssi\": %d }",
             WiFi.RSSI());
    request->send(200, MIMETYPE_JSON, _rssiBuffer);
}
