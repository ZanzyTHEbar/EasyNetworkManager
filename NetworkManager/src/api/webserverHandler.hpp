#pragma once
#ifndef XWEBSERVERHANDLER_HPP
#define XWEBSERVERHANDLER_HPP
#include "api/baseAPI/baseAPI.hpp"

class APIServer : public BaseAPI
{
private:
    /* Handlers */
    void handleRequest(AsyncWebServerRequest *request);
    void handleUserCommands(AsyncWebServerRequest *request);

public:
    APIServer(int CONTROL_PORT,
              ProjectConfig *configManager,
              const std::string &api_url,
              const std::string &wifimanager_url,
              const std::string &userCommands);

    virtual ~APIServer();
    void begin();
    void setupServer();

    void findParam(AsyncWebServerRequest *request, const char *param, String &value);
    void updateCommandHandlers(const std::string &url, stateFunction_t funct);
    void updateCommandHandlers(const std::string &url, stateFunction_request_t funct);
    void addRouteMap(const std::string &index, route_t route, std::vector<std::string> &indexes);

public:
    std::vector<std::string> indexes;
};
#endif // WEBSERVERHANDLER_HPP
