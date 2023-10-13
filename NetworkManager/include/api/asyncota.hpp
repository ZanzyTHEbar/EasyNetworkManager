#pragma once

#include "api/server.hpp"
#include "data/config/project_config.hpp"
#include "utilities/api_utilities.hpp"

class AsyncOTA {
    typedef std::function<void(void)> AsyncOTACustomHandlerFunction;
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
    AsyncOTACustomHandlerFunction customHandlerFunction = NULL;

    bool _authRequired;
};
