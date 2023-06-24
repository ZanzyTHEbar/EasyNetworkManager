#pragma once
#ifndef XWEBSERVERHANDLER_HPP
#    define XWEBSERVERHANDLER_HPP
#    include "api/base/base_api.hpp"

class APIServer : public BaseAPI {
   private:
    /* Handlers */
    void handleRequest(AsyncWebServerRequest* request);
    void handleUserCommands(AsyncWebServerRequest* request);
    void setupCaptivePortal();

   public:
    APIServer(const int CONTROL_PORT, ProjectConfig& configManager,
              const std::string& api_url, const std::string& wifimanager_url,
              const std::string& userCommands);

    virtual ~APIServer();
    void begin();
    void setupServer();

    bool findParam(AsyncWebServerRequest* request, const std::string& param,
                   std::string& value);
    void addAPICommand(const std::string& url, ArRequestHandlerFunction funct);
    void addRouteMap(const std::string& index, route_t route,
                     std::vector<std::string>& indexes);

   public:
    std::vector<std::string> indexes;
};
#endif  // WEBSERVERHANDLER_HPP
