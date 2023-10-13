#ifndef BASEAPI_HPP
#define BASEAPI_HPP

#include <string>
#include <unordered_map>

#include "data/config/project_config.hpp"
#include "utilities/api_utilities.hpp"
#include "utilities/helpers.hpp"

class BaseAPI : public API_Utilities {
   protected:
    ProjectConfig& configManager;

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

    using route_method = void (BaseAPI::*)(AsyncWebServerRequest*);
    using route_t = std::unordered_map<std::string, route_method>;
    using route_map_t = std::unordered_map<std::string, route_t>;

    route_t routes;
    route_map_t route_map;

   public:
    BaseAPI(ProjectConfig& configManager);
    virtual ~BaseAPI();

    std::unordered_map<std::string, ArRequestHandlerFunction> stateFunctionMap;
};

#endif  // BASEAPI_HPP