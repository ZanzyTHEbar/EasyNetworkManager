#ifndef BASEAPI_HPP
#define BASEAPI_HPP

#include <unordered_map>
#include <string>
#include <sstream>

#define WEBSERVER_H

constexpr int HTTP_GET = 0b00000001;
constexpr int HTTP_POST = 0b00000010;
constexpr int HTTP_DELETE = 0b00000100;
constexpr int HTTP_PUT = 0b00001000;
constexpr int HTTP_PATCH = 0b00010000;
constexpr int HTTP_HEAD = 0b00100000;
constexpr int HTTP_OPTIONS = 0b01000000;
constexpr int HTTP_ANY = 0b01111111;

#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

#include "data/config/project_config.hpp"
#include "data/utilities/helpers.hpp"
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

    std::unordered_map<WebRequestMethodComposite, std::string> _networkMethodsMap = {
        {HTTP_GET, "GET"},
        {HTTP_POST, "POST"},
        {HTTP_PUT, "PUT"},
        {HTTP_DELETE, "DELETE"},
        {HTTP_PATCH, "PATCH"},
        {HTTP_OPTIONS, "OPTIONS"},
    };

    std::unordered_map<std::string, WebRequestMethodComposite> _networkMethodsMap_inv = {
        {"GET", HTTP_GET},
        {"POST", HTTP_POST},
        {"PUT", HTTP_PUT},
        {"DELETE", HTTP_DELETE},
        {"PATCH", HTTP_PATCH},
        {"OPTIONS", HTTP_OPTIONS},
    };

    enum RequestMethods
    {
        GET,
        POST,
        PUT,
        DELETE,
        PATCH,
        OPTIONS
    };

    std::unordered_map<WebRequestMethodComposite, RequestMethods> _networkMethodsMap_enum = {
        {HTTP_GET, GET},
        {HTTP_POST, POST},
        {HTTP_PUT, PUT},
        {HTTP_DELETE, DELETE},
        {HTTP_PATCH, PATCH},
        {HTTP_OPTIONS, OPTIONS},
    };

protected:
    /* Commands */
    void setWiFi(AsyncWebServerRequest *request);
    void setWiFiTXPower(AsyncWebServerRequest *request);
    void handleJson(AsyncWebServerRequest *request);
    void factoryReset(AsyncWebServerRequest *request);
    void rebootDevice(AsyncWebServerRequest *request);
    void removeRoute(AsyncWebServerRequest *request);
    void getJsonConfig(AsyncWebServerRequest *request);
    void ping(AsyncWebServerRequest *request);
    void save(AsyncWebServerRequest *request);
    /* Helpers */
    void notFound(AsyncWebServerRequest *request) const;

    /* Route Command types */
    using route_method = void (BaseAPI::*)(AsyncWebServerRequest *);
    typedef std::unordered_map<std::string, route_method> route_t;
    typedef std::unordered_map<std::string, route_t> route_map_t;

    route_t routes;
    route_map_t route_map;

protected:
    AsyncWebServer *server;
    ProjectConfig *configManager;
    typedef std::unordered_map<std::string, WebRequestMethodComposite> networkMethodsMap_t;

    std::string api_url;
    std::string wifimanager_url;
    std::string userCommands;

    static bool ssid_write;
    static bool pass_write;
    static bool channel_write;

public:
    BaseAPI(int CONTROL_PORT,
            ProjectConfig *configManager,
            const std::string &api_url,
            const std::string &wifimanager_url,
            const std::string &userCommands);
    virtual ~BaseAPI();
    virtual void begin();

public:
    typedef void (*stateFunction_t)(void);
    typedef void (*stateFunction_request_t)(AsyncWebServerRequest *request);
    std::unordered_map<std::string, stateFunction_t> stateFunctionMap;
    std::unordered_map<std::string, stateFunction_request_t> stateFunctionMapRequest;

    struct userRoutes_t
    {
        // create a constructor to initialize the variables
        userRoutes_t(const std::string &endpoint,
                     const std::string &file,
                     const std::string &method) : endpoint(std::move(endpoint)),
                                                  file(std::move(file)),
                                                  method(std::move(method)) {}
        std::string endpoint;
        std::string file;
        std::string method;
    };
    std::vector<userRoutes_t> userEndpointsVector;
};

#endif // BASEAPI_HPP