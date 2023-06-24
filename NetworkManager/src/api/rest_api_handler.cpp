#include <api/rest_api_handler.hpp>

//*********************************************************************************************
//!                                     API Server
//*********************************************************************************************

APIServer::APIServer(const int CONTROL_PORT, ProjectConfig& configManager,
                     const std::string& api_url,
                     const std::string& wifimanager_url,
                     const std::string& userCommands)
    : BaseAPI(CONTROL_PORT, configManager, api_url, wifimanager_url,
              userCommands) {}

APIServer::~APIServer() {}

void APIServer::begin() {
    log_d("[APIServer]: Initializing REST API");
    this->setupServer();
    BaseAPI::begin();

    char buffer[1000];
    snprintf(buffer, sizeof(buffer), "^\\%s\\/([a-zA-Z0-9]+)\\/([a-zA-Z0-9]+)$",
             this->api_url.c_str());
    log_d("[APIServer]: API URL: %s", buffer);
    server.on(buffer, XHTTP_ANY,
              [&](AsyncWebServerRequest* request) { handleRequest(request); });

    char buf[1000];
    snprintf(buf, sizeof(buf), "^\\%s\\/([a-zA-Z0-9]+)\\/([a-zA-Z0-9]+)$",
             this->wifimanager_url.c_str());
    server.on(buf, HTTP_ANY,
              [&](AsyncWebServerRequest* request) { handleRequest(request); });
#ifdef USE_ASYNCOTA
    beginOTA();
#endif  // USE_ASYNCOTA
    server.begin();
}

void APIServer::setupServer() {
    // Set default routes
    routes.reserve(10);  // reserve enough memory for all routes
    routes.emplace("wifi", &APIServer::setWiFi);
    routes.emplace("json", &APIServer::handleJson);
    routes.emplace("resetConfig", &APIServer::factoryReset);
    routes.emplace("getConfig", &APIServer::getJsonConfig);
    routes.emplace("deleteRoute", &APIServer::removeRoute);
    routes.emplace("rebootDevice", &APIServer::rebootDevice);
    routes.emplace("ping", &APIServer::ping);
    routes.emplace("save", &APIServer::save);
    routes.emplace("wifiStrength", &APIServer::rssi);

    //! reserve enough memory for all routes - must be called after adding
    //! routes and before adding routes to route_map
    indexes.reserve(routes.size());  // this is done to avoid reallocation of
                                     // memory and copying of data
    addRouteMap("builtin", routes,
                indexes);  // add new route map to the route_map
}

/**
 * @brief Find a parameter in the request
 * @note This is a utility function to find a parameter in the request
 * @note This function modifies the value parameter, returns a boolean
 * @param request \c AsyncWebServerRequest* the request object
 * @param param \c std::string the parameter to find
 * @param value \c std::string& the value of the parameter
 * @return bool \c true if the parameter is found, \c false otherwise
 */
bool APIServer::findParam(AsyncWebServerRequest* request,
                          const std::string& param, std::string& value) {
    if (request->hasParam(param.c_str())) {
        value.assign(request->getParam(param.c_str(), true)->value().c_str());
        return true;
    }
    return false;
}

/**
 * @brief Add a command handler to the API
 *
 * @param index
 * @param funct
 * @param indexes \c std::vector<std::string> a list of the routes of the
 * command handlers
 *
 * @return void
 *
 */
void APIServer::addRouteMap(const std::string& index, route_t route,
                            std::vector<std::string>& indexes) {
    route_map.emplace(std::move(index), route);

    for (const auto& key : route) {
        indexes.emplace_back(key.first);  // add the route to the list of routes
                                          // - use emplace_back to avoid copying
    }
}

void APIServer::handleRequest(AsyncWebServerRequest* request) {
    std::vector<std::string> temp = Helpers::split(userCommands.c_str(), '/');

    if (strcmp(request->pathArg(0).c_str(), temp[1].c_str()) == 0) {
        handleUserCommands(request);
        return;
    }

    // Get the route
    log_i("Request URL: %s", request->url().c_str());
    log_i("Request: %s", request->pathArg(0).c_str());
    log_i("Request: %s", request->pathArg(1).c_str());

    auto it_map = route_map.find(request->pathArg(0).c_str());
    auto it_method = it_map->second.find(request->pathArg(1).c_str());

    if (it_map != route_map.end()) {
        if (it_method != it_map->second.end()) {
            log_d("[APIServer]: We are trying to execute the function");
            (*this.*(it_method->second))(request);
            return;
        }
        log_e("[APIServer]: Invalid Command");
        request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Command\"}");
        return;
    }
    log_e("[APIServer]: Invalid Map Index");
    request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Map Index\"}");
}

void APIServer::addAPICommand(const std::string& url,
                              ArRequestHandlerFunction funct) {
    stateFunctionMap.emplace(std::move(url), funct);
}

/**
 * @brief Add a command handler to the API
 *
 * @param request
 * @return \c void
 * @note This function is called by the API server when a command is received
 * @warning  \c This function requires the user to access the index using a url
 * parameter \c we need to fix this!! I need a better implemenation
 *
 */
void APIServer::handleUserCommands(AsyncWebServerRequest* request) {
    std::string url = request->pathArg(1).c_str();
    auto it = stateFunctionMap.find(url);
    if (it != stateFunctionMap.end()) {
        log_d("[APIServer]: We are trying to execute the function");
        it->second(request);
        return;
    }
    log_e("[APIServer]: Invalid Command");
    request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Command\"}");
}
