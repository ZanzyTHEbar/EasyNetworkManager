#pragma once
#ifndef XWEBSERVERHANDLER_HPP
#define XWEBSERVERHANDLER_HPP

#include "api/baseAPI/baseAPI.hpp"

class APIServer : public BaseAPI
{
private:
    /* Handlers */
    void command_handler(AsyncWebServerRequest *request);
    void wifi_command_handler(AsyncWebServerRequest *request);

public:
    APIServer(int CONTROL_PORT,
              WiFiHandler *network,
              DNSServer *dnsServer,
              std::string api_url,
              std::string wifimanager_url);

    virtual ~APIServer();
    void begin();
    void setupServer();

    void findParam(AsyncWebServerRequest *request, const char *param, String &value);
    void updateCommandHandlers();
    std::vector<std::string> routeHandler(std::string index, route_t route);
    void routeHandler(std::string index, AsyncWebServerRequest *request);
    void handleRequest(AsyncWebServerRequest *request);
};
#endif // WEBSERVERHANDLER_HPP
