#pragma once

#include <data/config/project_config.hpp>
#include <utilities/api_utilities.hpp>
#include "server.hpp"

using AsyncOTACustomHandlerFunction = std::function<void(void)>;
class AsyncOTA {
    AsyncOTACustomHandlerFunction customHandlerFunction = NULL;
    AsyncServer_t& async_server;

   protected:
    ProjectConfig& configManager;

   public:
    AsyncOTA(ProjectConfig& configManager, AsyncServer_t& async_server);
    virtual ~AsyncOTA();
    void begin();
    void checkAuthentication(AsyncWebServerRequest* request, const char* login,
                             const char* password);
    void setOTAHandler(AsyncOTACustomHandlerFunction customHandlerFunction);

    bool _authRequired;
};
