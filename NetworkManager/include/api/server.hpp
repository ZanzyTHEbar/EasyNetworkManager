#pragma once

// #define WEBSERVER_H
#include "utilities/api_utilities.hpp"

#ifdef ESP32
#    include <AsyncTCP.h>
#elif defined(ESP8266)
#    include <ESPAsyncTCP.h>
#endif

#ifdef USE_WEBMANAGER
#    include <data/webpage.h>
#endif  // USE_WEBMANAGER

#include "data/config/project_config.hpp"

class AsyncServer_t : public API_Utilities {
    /* Helpers */
    void notFound(AsyncWebServerRequest* request) const;

   protected:
    ProjectConfig& configManager;

   public:
    AsyncServer_t(const int CONTROL_PORT, ProjectConfig& configManager,
                  const std::string& api_url,
                  const std::string& wifimanager_url,
                  const std::string& userCommands);
    virtual ~AsyncServer_t();
    virtual void begin();

    struct userRoutes_t {
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

    std::string api_url;
    std::string wifimanager_url;
    std::string userCommands;
};