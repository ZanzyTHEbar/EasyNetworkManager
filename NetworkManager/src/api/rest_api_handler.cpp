#include <api/rest_api_handler.hpp>

//*********************************************************************************************
//!                                     API Server
//*********************************************************************************************

APIServer::APIServer(ProjectConfig& configManager, AsyncServer_t& async_server,
                     AsyncOTA* async_ota)
    : BaseAPI(configManager), async_ota(nullptr), async_server(async_server) {
    if (async_ota != nullptr) {
        this->async_ota = async_ota;
    }
}

APIServer::~APIServer() {
    if (async_ota != nullptr) {
        delete async_ota;
    }
}

void APIServer::begin() {
    log_d("[APIServer]: Initializing REST API");
    this->setupServer();
    async_server.begin();
    char api_url[1000];
    char wifi_manager_url[1000];
    snprintf(api_url, sizeof(api_url),
             "^\\%s\\/([a-zA-Z0-9]+)\\/([a-zA-Z0-9]+)$",
             async_server.api_url.c_str());
    snprintf(wifi_manager_url, sizeof(wifi_manager_url),
             "^\\%s\\/([a-zA-Z0-9]+)\\/([a-zA-Z0-9]+)$",
             async_server.wifimanager_url.c_str());

    log_d("[APIServer]: API URL: %s", api_url);

    async_server.server.on(
        api_url, XHTTP_ANY,
        [&](AsyncWebServerRequest* request) { handleRequest(request); });

    async_server.server.on(
        wifi_manager_url, HTTP_ANY,
        [&](AsyncWebServerRequest* request) { handleRequest(request); });

    if (async_ota != nullptr)
        async_ota->begin();
    async_server.server.begin();
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
    addRouteMap("builtin", routes);  // add new route map to the route_map
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
void APIServer::addRouteMap(const std::string& index, route_t route) {
    route_map.emplace(std::move(index), route);

    for (const auto& key : route) {
        const std::string temp =
            index + '/' + key.first;  // add the route to the index
        indexes.emplace_back(temp);   // add the route to the list of routes
                                      // - use emplace_back to avoid copying
    }
}

void APIServer::handleRequest(AsyncWebServerRequest* request) {
    std::vector<std::string> temp =
        Helpers::split(async_server.userCommands.c_str(), '/');

    if (strcmp(request->pathArg(0).c_str(), temp[1].c_str()) == 0) {
        handleUserCommands(request);
        return;
    }

    // Get the route
    log_i("Request URL: %s", request->url().c_str());
    log_i("Request: %s", request->pathArg(0).c_str());
    log_i("Request: %s", request->pathArg(1).c_str());

    auto it_map = route_map.find(request->pathArg(0).c_str());

    if (it_map == route_map.end()) {
        log_e("[APIServer]: Invalid Command");
        request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Command\"}");
        return;
    }

    auto it_method = it_map->second.find(request->pathArg(1).c_str());

    if (it_method == it_map->second.end()) {
        log_e("[APIServer]: Invalid Map Index");
        request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Map Index\"}");
        return;
    }

    log_d("[APIServer]: We are trying to execute the function");
    (*this.*(it_method->second))(request);
    request->send(200);
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

void APIServer::setupCaptivePortal(bool apMode) {
    log_d("[SETUP Captive Portal]: Starting Captive Portal");
    log_d("[SETUP Captive Portal]: AP Mode: %s",
          apMode ? "enabled" : "disabled");
    async_server.server.addHandler(new CaptiveRequestHandler(async_server))
        .setFilter(apMode ? ON_AP_FILTER : ON_STA_FILTER);
}

//*********************************************************************************************
//!                                     API Utilities
//*********************************************************************************************

CaptiveRequestHandler::CaptiveRequestHandler(AsyncServer_t& async_server)
    : async_server(async_server) {}

CaptiveRequestHandler::~CaptiveRequestHandler() {}

bool CaptiveRequestHandler::canHandle(AsyncWebServerRequest* request) {
    // request->addInterestingHeader("ANY");
    return true;
}

void CaptiveRequestHandler::handleRequest(AsyncWebServerRequest* request) {
    if (async_server.spiffsMounted) {
        if (async_server.custom_html_files.size() > 0) {
            AsyncResponseStream* response_stream =
                request->beginResponseStream("text/html");
            for (auto& file : async_server.custom_html_files) {
                if (file.endpoint == "captive_portal" ||
                    file.endpoint == "captive_portal.html" ||
                    file.endpoint == "/" || file.endpoint == "/index.html" ||
                    file.endpoint == "/index" ||
                    file.endpoint == "portal.html") {
                    response_stream->print(file.file.c_str());
                }
            }
            request->send(response_stream);
            return;
        }
    }

    AsyncWebServerResponse* response = request->beginResponse_P(
        200, "text/html", WEB_MANAGER_HTML, WEB_MANAGER_HTML_SIZE);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}