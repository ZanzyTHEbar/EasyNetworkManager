#ifndef BASEAPI_HPP
#define BASEAPI_HPP

#include <stdlib_noniso.h>
#include <string>
#include <unordered_map>

#ifdef USE_WEBMANAGER
#    include <data/webpage.h>
#endif

// #define WEBSERVER_H

constexpr int XHTTP_GET = 0b00000001;
constexpr int XHTTP_POST = 0b00000010;
constexpr int XHTTP_DELETE = 0b00000100;
constexpr int XHTTP_PUT = 0b00001000;
constexpr int XHTTP_PATCH = 0b00010000;
constexpr int XHTTP_HEAD = 0b00100000;
constexpr int XHTTP_OPTIONS = 0b01000000;
constexpr int XHTTP_ANY = 0b01111111;

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
            {"GET", XHTTP_GET},     {"POST", XHTTP_POST},
            {"PUT", XHTTP_PUT},     {"DELETE", XHTTP_DELETE},
            {"PATCH", XHTTP_PATCH}, {"OPTIONS", XHTTP_OPTIONS},
        };

    std::unordered_map<WebRequestMethodComposite, std::string>
        _networkMethodsMap = {
            {XHTTP_GET, "GET"},     {XHTTP_POST, "POST"},
            {XHTTP_PUT, "PUT"},     {XHTTP_DELETE, "DELETE"},
            {XHTTP_PATCH, "PATCH"}, {XHTTP_OPTIONS, "OPTIONS"},
        };

    enum RequestMethods { GET, POST, PUT, DELETE, PATCH, OPTIONS };

    std::unordered_map<WebRequestMethodComposite, RequestMethods>
        _networkMethodsMap_enum = {
            {XHTTP_GET, GET},     {XHTTP_POST, POST},
            {XHTTP_PUT, PUT},     {XHTTP_DELETE, DELETE},
            {XHTTP_PATCH, PATCH}, {XHTTP_OPTIONS, OPTIONS},
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
    typedef std::function<void(void)> AsyncOTACustomHandlerFunction;
    void setOTAHandler(AsyncOTACustomHandlerFunction customHandlerFunction);
    AsyncOTACustomHandlerFunction customHandlerFunction = NULL;
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
    AsyncWebServer server;
    bool spiffsMounted;
};

#endif  // BASEAPI_HPP