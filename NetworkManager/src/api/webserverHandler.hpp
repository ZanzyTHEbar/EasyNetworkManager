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
              WiFiHandler *network,
              DNSServer *dnsServer,
              std::string api_url,
              std::string wifimanager_url,
              std::string userCommands);

    virtual ~APIServer();
    void begin();
    void setupServer();

    void findParam(AsyncWebServerRequest *request, const char *param, String &value);
    void updateCommandHandlers(std::string url, stateFunction_t funct);
    std::vector<std::string> routeHandler(std::string index, route_t route);
    void routeHandler(std::string index, AsyncWebServerRequest *request);
};
#endif // WEBSERVERHANDLER_HPP
