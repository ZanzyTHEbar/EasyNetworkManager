#ifndef BASEAPI_HPP
#define BASEAPI_HPP

#include <stdlib_noniso.h>
#include <string>
#include <unordered_map>

#ifdef USE_WEBMANAGER
#    include <data/webpage.h>
#endif

#define WEBSERVER_H

constexpr int HTTP_GET = 0b00000001;
constexpr int HTTP_POST = 0b00000010;
constexpr int HTTP_DELETE = 0b00000100;
constexpr int HTTP_PUT = 0b00001000;
constexpr int HTTP_PATCH = 0b00010000;
constexpr int HTTP_HEAD = 0b00100000;
constexpr int HTTP_OPTIONS = 0b01000000;
constexpr int HTTP_ANY = 0b01111111;

#include <Update.h>
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>

#include <ESPAsyncWebServer.h>
#include <FS.h>
#include "Hash.h"

#ifdef ESP32
#    include <AsyncTCP.h>
#elif defined(ESP8266)
#    include <ESPAsyncTCP.h>
#endif

#include "data/config/project_config.hpp"
#include "elegantWebpage.h"
#include "utilities/api_utilities.hpp"
#include "utilities/helpers.hpp"

class BaseAPI : public API_Utilities {
   protected:
    /* Commands */
    void setWiFi(AsyncWebServerRequest* request);
    void setWiFiTXPower(AsyncWebServerRequest* request);
    void handleJson(AsyncWebServerRequest* request);
    void factoryReset(AsyncWebServerRequest* request);
    void rebootDevice(AsyncWebServerRequest* request);
    void removeRoute(AsyncWebServerRequest* request);
    void getJsonConfig(AsyncWebServerRequest* request);
    void ping(AsyncWebServerRequest* request);
    void save(AsyncWebServerRequest* request);
    void rssi(AsyncWebServerRequest* request);

    /* Helpers */
    void notFound(AsyncWebServerRequest* request) const;

    /* Route Command types */
    using route_method = void (BaseAPI::*)(AsyncWebServerRequest*);
    typedef std::unordered_map<std::string, route_method> route_t;
    typedef std::unordered_map<std::string, route_t> route_map_t;

    route_t routes;
    route_map_t route_map;

   protected:
    /// @brief Local instance of the AsyncWebServer - so that we dont need to
    /// use new and delete
    AsyncWebServer server;
    ProjectConfig& configManager;

    std::string api_url;
    std::string wifimanager_url;
    std::string userCommands;
    bool _authRequired;

    typedef std::unordered_map<std::string, WebRequestMethodComposite>
        networkMethodsMap_t;

   public:
    std::unordered_map<std::string, WebRequestMethodComposite>
        _networkMethodsMap_inv = {
            {"GET", HTTP_GET},     {"POST", HTTP_POST},
            {"PUT", HTTP_PUT},     {"DELETE", HTTP_DELETE},
            {"PATCH", HTTP_PATCH}, {"OPTIONS", HTTP_OPTIONS},
        };

    std::unordered_map<WebRequestMethodComposite, std::string>
        _networkMethodsMap = {
            {HTTP_GET, "GET"},     {HTTP_POST, "POST"},
            {HTTP_PUT, "PUT"},     {HTTP_DELETE, "DELETE"},
            {HTTP_PATCH, "PATCH"}, {HTTP_OPTIONS, "OPTIONS"},
        };

    enum RequestMethods { GET, POST, PUT, DELETE, PATCH, OPTIONS };

    std::unordered_map<WebRequestMethodComposite, RequestMethods>
        _networkMethodsMap_enum = {
            {HTTP_GET, GET},       {HTTP_POST, POST},   {HTTP_PUT, PUT},
            {HTTP_DELETE, DELETE}, {HTTP_PATCH, PATCH}, {HTTP_OPTIONS, OPTIONS},
        };

   public:
    BaseAPI(const int CONTROL_PORT, ProjectConfig& configManager,
            const std::string& api_url, const std::string& wifimanager_url,
            const std::string& userCommands);
    virtual ~BaseAPI();
    virtual void begin();
#ifdef USE_ASYNCOTA
    void checkAuthentication(AsyncWebServerRequest* request, const char* login,
                             const char* password);
    void beginOTA();
#endif  // USE_ASYNCOTA
   public:
    std::unordered_map<std::string, ArRequestHandlerFunction> stateFunctionMap;

    struct userRoutes_t {
        // create a constructor to initialize the variables
        userRoutes_t(const std::string& endpoint, const std::string& file,
                     const std::string& method)
            : endpoint(std::move(endpoint)),
              file(std::move(file)),
              method(std::move(method)) {}
        std::string endpoint;
        std::string file;
        std::string method;
    };
    std::vector<userRoutes_t> custom_html_files;
};

#endif  // BASEAPI_HPP