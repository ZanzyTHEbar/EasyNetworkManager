#include "baseAPI.hpp"

BaseAPI::BaseAPI(int CONTROL_PORT,
                 WiFiHandler *network,
                 DNSServer *dnsServer,
                 std::string api_url,
                 std::string wifimanager_url) : API_Utilities(CONTROL_PORT, network, dnsServer, api_url, wifimanager_url) {}

BaseAPI::~BaseAPI() {}

void BaseAPI::begin()
{
    this->setupServer();
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

    // std::bind(&BaseAPI::API_Utilities::notFound, &api_utilities, std::placeholders::_1);
    server->onNotFound([&](AsyncWebServerRequest *request)
                       { notFound(request); });
}

void BaseAPI::setupServer()
{
    localWifiConfig = {
        .ssid = "",
        .pass = "",
        .channel = 0,
    };

    localAPWifiConfig = {
        .ssid = "",
        .pass = "",
        .channel = 0,
    };

    // command_map_wifi_conf.emplace("ssid", &BaseAPI::setWiFi);
    // command_map_method.emplace("reset_config", &BaseAPI::factoryReset);
    // command_map_method.emplace("reboot_device", &BaseAPI::rebootDevice);
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
        for (int i = 0; i < params; i++)
        {
            AsyncWebParameter *param = request->getParam(i);
            if (network->stateManager->getCurrentState() == WiFiState_e::WiFiState_ADHOC)
            {
                localAPWifiConfig.ssid = param->value().c_str();
                localAPWifiConfig.pass = param->value().c_str();
                localAPWifiConfig.channel = atoi(param->value().c_str());
            }
            else
            {
                localWifiConfig.ssid = param->value().c_str();
                localWifiConfig.pass = param->value().c_str();
                localWifiConfig.channel = atoi(param->value().c_str());
            }
            log_i("%s[%s]: %s\n", _networkMethodsMap[request->method()].c_str(), param->name().c_str(), param->value().c_str());
        }
        ssid_write = true;
        pass_write = true;
        channel_write = true;
        request->send(200, MIMETYPE_JSON, "{\"msg:\"\"Done. Wifi Creds have been set.\"}");
        request->redirect("/");
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

/**
 * * Trigger in main loop to save config to flash
 * ? Should we force the users to update all config params before triggering a config write?
 */
void BaseAPI::triggerWifiConfigWrite()
{
    if (ssid_write && pass_write && channel_write)
    {
        ssid_write = false;
        pass_write = false;
        channel_write = false;
        if (network->stateManager->getCurrentState() == WiFiState_e::WiFiState_ADHOC)
            network->configManager->setWifiConfig(localAPWifiConfig.ssid.c_str(), localAPWifiConfig.ssid.c_str(), localAPWifiConfig.pass.c_str(), &localAPWifiConfig.channel, true);
        else
            network->configManager->setWifiConfig(localWifiConfig.ssid.c_str(), localWifiConfig.ssid.c_str(), localWifiConfig.pass.c_str(), &localWifiConfig.channel, true);
        network->configManager->save();
    }
}

void BaseAPI::handleJson(AsyncWebServerRequest *request)
{
    std::string type = request->pathArg(0).c_str();
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
            String temp = network->configManager->getDeviceConfig()->data_json_string;
            request->send(200, MIMETYPE_JSON, temp);
            temp = "";
            break;
        }
        case SETTINGS:
        {
            network->configManager->getDeviceConfig()->config_json = true;
            Network_Utilities::my_delay(1L);
            String temp = network->configManager->getDeviceConfig()->config_json_string;
            request->send(200, MIMETYPE_JSON, temp);
            temp = "";
            break;
        }
        case CONFIG:
        {
            network->configManager->getDeviceConfig()->settings_json = true;
            Network_Utilities::my_delay(1L);
            String temp = network->configManager->getDeviceConfig()->settings_json_string;
            request->send(200, MIMETYPE_JSON, temp);
            temp = "";
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
        delay(20000);
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
        network->configManager->reset();
        request->send(200, MIMETYPE_JSON, "{\"msg\":\"Factory Reset\"}");
    }
    default:
    {
        request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Request\"}");
        break;
    }
    }
}

/**
 * @brief This function is used to set the callback functions for the API
 * @attention This function must be called before begin()
 * @param callback
 */
/* void BaseAPI::setCallback(call_back_function_t2 callback)
{
    this->function_callback = callback;
}
 */