#ifndef BASEAPI_HPP
#define BASEAPI_HPP
#include "wifihandler/wifiHandler.hpp"
#include "api/utilities/apiUtilities.hpp"

class BaseAPI : public API_Utilities
{
protected:

    enum JSON_TYPES
    {
        CONFIG,
        SETTINGS,
        DATA,
        STATUS,
        COMMANDS,
        WIFI,
        WIFIAP,
    };

    std::unordered_map<std::string, JSON_TYPES> json_TypesMap = {
        {"config", CONFIG},
        {"settings", SETTINGS},
        {"data", DATA},
        {"status", STATUS},
        {"commands", COMMANDS},
        {"wifi", WIFI},
        {"wifiap", WIFIAP},
    };

protected:
    /* Commands */
    void setWiFi(AsyncWebServerRequest *request);
    void handleJson(AsyncWebServerRequest *request);
    void factoryReset(AsyncWebServerRequest *request);
    void rebootDevice(AsyncWebServerRequest *request);
    void removeRoute(AsyncWebServerRequest *request);
    

    /* Route Command types */
    using route_method = void (BaseAPI::*)(AsyncWebServerRequest *);
    typedef std::unordered_map<std::string, route_method> route_t;
    typedef std::unordered_map<std::string, route_t> route_map_t;

    route_t routes;
    route_map_t route_map;

public:
    BaseAPI(int CONTROL_PORT,
            WiFiHandler *network,
            DNSServer *dnsServer,
            const std::string &api_url,
            const std::string &wifimanager_url,
            const std::string &userCommands);
    virtual ~BaseAPI();
    virtual void begin();
    void loop();
    void handle();

public:
    typedef void (*stateFunction_t)(void);
    std::unordered_map<std::string, stateFunction_t> stateFunctionMap;
};

#endif // BASEAPI_HPP