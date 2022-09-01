#include "baseAPI.hpp"

BaseAPI::BaseAPI(int CONTROL_PORT,
                 WiFiHandler *network,
                 DNSServer *dnsServer,
                 const std::string &api_url,
                 const std::string &wifimanager_url,
                 const std::string &userCommands) : API_Utilities(CONTROL_PORT,
                                                                  network,
                                                                  dnsServer,
                                                                  api_url,
                                                                  wifimanager_url,
                                                                  userCommands) {}

BaseAPI::~BaseAPI() {}

void BaseAPI::begin()
{
    //! i have changed this to use lambdas instead of std::bind to avoid the overhead. Lambdas are always more preferable.
    server->on("/", HTTP_GET, [&](AsyncWebServerRequest *request)
               { request->send(200); });

    if (initSPIFFS())
    {
        /* server->on(wifimanager_url.c_str(), HTTP_GET, [&](AsyncWebServerRequest *request)
                   { request->send(SPIFFS, "/wifimanager.html", MIMETYPE_HTML); }); */

        server->serveStatic(wifimanager_url.c_str(), SPIFFS, "/wifimanager.html").setCacheControl("max-age=600");
    }

    // preflight cors check
    server->on("/", HTTP_OPTIONS, [&](AsyncWebServerRequest *request)
               {
        		AsyncWebServerResponse* response = request->beginResponse(204);
        		response->addHeader("Access-Control-Allow-Methods", "PUT,POST,GET,OPTIONS");
        		response->addHeader("Access-Control-Allow-Headers", "Accept, Content-Type, Authorization, FileSize");
        		response->addHeader("Access-Control-Allow-Credentials", "true");
        		request->send(response); });

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

    server->onNotFound([&](AsyncWebServerRequest *request)
                       { notFound(request); });
}

//*********************************************************************************************
//!                                     Command Functions
//*********************************************************************************************
void BaseAPI::setWiFi(AsyncWebServerRequest *request)
{
    switch (_networkMethodsMap_enum[request->method()])
    {
    case POST:
    {
        int params = request->params();

        std::string ssid;
        std::string password;
        std::string mdns_url;
        uint8_t channel = 0;

        int OTA_port = 0;
        std::string OTA_password;

        log_d("Number of Params: %d", params);
        for (int i = 0; i < params; i++)
        {
            AsyncWebParameter *param = request->getParam(i);
            if (param->name() == "ssid")
            {
                ssid = param->value().c_str();
            }
            else if (param->name() == "password")
            {
                password = param->value().c_str();
            }
            else if (param->name() == "channel")
            {
                channel = (uint8_t)atoi(param->value().c_str());
            }
            else if (param->name() == "mdns")
            {
                mdns_url = param->value().c_str();
            }
            else if (param->name() == "ota_port")
            {
                OTA_port = atoi(param->value().c_str());
            }
            else if (param->name() == "ota_password")
            {
                OTA_password = param->value().c_str();
            }
            log_i("%s[%s]: %s\n", _networkMethodsMap[request->method()].c_str(), param->name().c_str(), param->value().c_str());
        }

        network->configManager->setWifiConfig(ssid, ssid, password, &channel, true);

        // check if the ota and mdns are set
        if (!OTA_password.empty())
        {
            int OTAPort = OTA_port > 0 ? OTA_port : 3232;
            network->configManager->setDeviceConfig(OTA_password, &OTAPort, true);
        }

        if (!mdns_url.empty())
        {
            network->configManager->setMDNSConfig(mdns_url, true);
        }

        //! TODO: implement user-costumizable ADHOC network
        /* if (network->stateManager->getCurrentState() == WiFiState_e::WiFiState_ADHOC)
        {
        }
        else
        {
        } */

        request->send(200, MIMETYPE_JSON, "{\"msg\":\"Done. Device Wifi settings have been set.\"}");
        network->configManager->save();
        break;
    }
    default:
    {
        request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Request\"}");
        request->redirect("/");
        break;
    }
    }
}

void BaseAPI::handleJson(AsyncWebServerRequest *request)
{
    const std::string &type = request->pathArg(0).c_str();
    switch (_networkMethodsMap_enum[request->method()])
    {
    case POST:
    {
        switch (json_TypesMap.at(type))
        {
        case DATA:
        {
            break;
        }
        case SETTINGS:
        {
            break;
        }
        case CONFIG:
        {
            break;
        }
        default:
            request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Request\"}");
            break;
        }
        break;
    }
    case GET:
    {
        switch (json_TypesMap.at(type))
        {
        case DATA:
        {
            network->configManager->getDeviceConfig()->data_json = true;
            Network_Utilities::my_delay(1L);
            std::string temp = network->configManager->getDeviceConfig()->data_json_string;
            request->send(200, MIMETYPE_JSON, temp.c_str());
            temp = std::string();
            break;
        }
        case SETTINGS:
        {
            network->configManager->getDeviceConfig()->config_json = true;
            Network_Utilities::my_delay(1L);
            std::string temp = network->configManager->getDeviceConfig()->config_json_string;
            request->send(200, MIMETYPE_JSON, temp.c_str());
            temp = std::string();
            break;
        }
        case CONFIG:
        {
            network->configManager->getDeviceConfig()->settings_json = true;
            Network_Utilities::my_delay(1L);
            std::string temp = network->configManager->getDeviceConfig()->settings_json_string;
            request->send(200, MIMETYPE_JSON, temp.c_str());
            temp = std::string();
            break;
        }
        default:
            request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Request\"}");
            break;
        }

        break;
    }
    default:
    {
        request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Request\"}");
        break;
    }
    }
}

void BaseAPI::rebootDevice(AsyncWebServerRequest *request)
{
    switch (_networkMethodsMap_enum[request->method()])
    {
    case GET:
    {
        request->send(200, MIMETYPE_JSON, "{\"msg\":\"Rebooting Device\"}");
        ESP.restart();
    }
    default:
    {
        request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Request\"}");
        break;
    }
    }
}

void BaseAPI::factoryReset(AsyncWebServerRequest *request)
{
    switch (_networkMethodsMap_enum[request->method()])
    {
    case GET:
    {
        log_d("Factory Reset");
        bool success = network->configManager->reset();
        char buf[100];
        snprintf(buf, sizeof(buf), "{\"msg\":\"Factory Reset - %s\"}", success ? "Done" : "Failed");
        request->send(200, MIMETYPE_JSON, buf);
    }
    default:
    {
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
void BaseAPI::removeRoute(AsyncWebServerRequest *request)
{
    log_i("Request: %s", request->url().c_str());
    int params = request->params();
    auto it_map = route_map.find(request->pathArg(0).c_str());
    log_i("Request: %s", request->pathArg(0).c_str());

    auto it = it_map->second.find(request->pathArg(1).c_str());
    if (it != it_map->second.end())
    {
        switch (_networkMethodsMap_enum[request->method()])
        {
        case DELETE:
        {
            it_map->second.erase(it);
            request->send(200, MIMETYPE_JSON, "{\"msg\":\"Route Removed\"}");
            break;
        }
        default:
        {
            request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Request\"}");
            break;
        }
        }
    }
    else
    {
        request->send(400, MIMETYPE_JSON, "{\"msg\":\"Route Not Found\"}");
    }
}
