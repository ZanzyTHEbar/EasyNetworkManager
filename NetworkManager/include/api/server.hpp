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
#include <data/config/project_config.hpp>

class AsyncServer_t : public API_Utilities {
    /* Helpers */
    void notFound(AsyncWebServerRequest* request) const;

   protected:
    ProjectConfig& configManager;

   public:
    AsyncServer_t(const int CONTROL_PORT, ProjectConfig& configManager,
                  const std::string& api_url,
                  const std::string& wifimanager_url,
                  const std::string& user_commands,
                  const std::string& json_url);
    virtual ~AsyncServer_t();
    virtual void begin();

    struct UserRoutes_t {
        UserRoutes_t(const std::string& endpoint, const std::string& file,
                     const std::string& method)
            : endpoint(std::move(endpoint)),
              file(std::move(file)),
              method(std::move(method)) {}
        std::string endpoint;
        std::string file;
        std::string method;
    };

    std::vector<UserRoutes_t> custom_html_files;

    AsyncWebServer server;
    bool spiffsMounted;

    std::string api_url;
    std::string wifimanager_url;
    std::string user_commands;
    std::string json_url;
};