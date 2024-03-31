#pragma once
#define XWEBSERVERHANDLER_HPP
#include "api/asyncota.hpp"
#include "api/base_api.hpp"
#include "api/server.hpp"

class APIServer : public BaseAPI {
   private:
    /* Handlers */
    void handleRequest(AsyncWebServerRequest* request);
    void handleuser_commands(AsyncWebServerRequest* request);

   public:
    APIServer(ProjectConfig& configManager, AsyncServer_t& async_server,
              AsyncOTA* async_ota = nullptr);
    virtual ~APIServer();
    void begin();
    void setupServer();

    bool findParam(AsyncWebServerRequest* request, const std::string& param,
                   std::string& value);
    void addAPICommand(const std::string& url, ArRequestHandlerFunction funct);
    void addJSONHandler(const std::string& endpoint,
                        AsyncCallbackJsonWebHandler* handler);
    void addRouteMap(const std::string& index, route_t route);
    void setupCaptivePortal(bool apMode = false);

    struct JSONRequest_t {
        std::string endpoint;
        AsyncCallbackJsonWebHandler* handler;

        JSONRequest_t(const std::string& endpoint,
                      AsyncCallbackJsonWebHandler* handler)
            : endpoint(endpoint), handler(handler) {}

        virtual ~JSONRequest_t() {
            delete handler;
        }
    };

   public:
    AsyncServer_t& async_server;
    AsyncOTA* async_ota;

    std::vector<std::string> indexes;
    std::unordered_map<std::string, JSONRequest_t*> jsonRequestHandlers;
};

//* Captive Portal Handler
class CaptiveRequestHandler : public AsyncWebHandler {
    AsyncServer_t& async_server;

   public:
    CaptiveRequestHandler(AsyncServer_t& async_server);
    virtual ~CaptiveRequestHandler();

    bool canHandle(AsyncWebServerRequest* request);

    //* use SPIFFS or built in HTML wifimanager
    void handleRequest(AsyncWebServerRequest* request);
};
