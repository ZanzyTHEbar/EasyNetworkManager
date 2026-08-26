#pragma once
#define XWEBSERVERHANDLER_HPP
#include <string>
#include <vector>
#include "asyncota.hpp"
#include "base_api.hpp"
#include "server.hpp"

class CaptiveRequestHandler;

class APIServer : public BaseAPI {
   private:
    /* Handlers */
    void handleRequest(AsyncWebServerRequest* request);
    void handle_user_commands(AsyncWebServerRequest* request);
    void handleProvisionGet(AsyncWebServerRequest* request);
    void handleProvisionPost(AsyncWebServerRequest* request);
    bool provisioningAvailable() const;
    std::string provisioningNonce;
    bool provisioningNonceUsed = false;
    CaptiveRequestHandler* captiveHandler = nullptr;
    std::vector<AsyncWebHandler*> registeredHandlers;
    bool started = false;

   public:
    // Non-owning references/pointer. AsyncServer_t and AsyncOTA must outlive
    // APIServer; AsyncServer_t owns server teardown. Destruction must not race
    // active requests.
    APIServer(ProjectConfig& configManager, AsyncServer_t& async_server,
              AsyncOTA* async_ota = nullptr);
    virtual ~APIServer();
    void begin();
    void setupServer();

    void addAPICommand(const std::string& url, ArRequestHandlerFunction funct);
    void addRouteMap(const std::string& index, route_t route);
    void setupCaptivePortal(bool apMode = false);

   public:
    AsyncServer_t& async_server;
    AsyncOTA* async_ota;

    std::vector<std::string> indexes;
};

//* Captive Portal Handler
class CaptiveRequestHandler : public AsyncWebHandler {
    AsyncServer_t& async_server;

   public:
    CaptiveRequestHandler(AsyncServer_t& async_server);
    virtual ~CaptiveRequestHandler();

    bool canHandle(AsyncWebServerRequest* request);

    //* use the configured filesystem or built in HTML wifimanager
    void handleRequest(AsyncWebServerRequest* request);
};
