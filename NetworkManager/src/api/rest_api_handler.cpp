#include <api/rest_api_handler.hpp>

#include <algorithm>
#include <cstdlib>
#include <utility>

#if defined(ESP32)
#    include <esp_system.h>
#elif defined(ESP8266)
#    include <user_interface.h>
#endif

namespace {
constexpr size_t kMaxSsidLength = 32;
constexpr size_t kMaxWifiPasswordLength = 63;
constexpr size_t kMinWpa2PasswordLength = 8;
constexpr size_t kMaxManagementLoginLength = 64;
constexpr size_t kMaxManagementPasswordLength = 64;
constexpr size_t kMaxProvisionBodyBytes = 1024;
constexpr uint8_t kDefaultProvisionChannel = 1;
constexpr uint8_t kMaxWifiChannel = 14;
constexpr char kLegacyDefaultApPassword[] = "12345678";

bool readPostParam(AsyncWebServerRequest* request, const char* name,
                   std::string& value, bool required) {
    value.clear();
    size_t matches = 0;
    for (size_t i = 0; i < request->params(); ++i) {
        const AsyncWebParameter* parameter = request->getParam(i);
        if (parameter != nullptr && parameter->isPost() &&
            !parameter->isFile() && parameter->name() == name) {
            value.assign(parameter->value().c_str());
            ++matches;
        }
    }

    if (matches > 1) {
        return false;
    }
    return !required || matches == 1;
}

bool validLength(const std::string& value, size_t minLength,
                 size_t maxLength) {
    return value.length() >= minLength && value.length() <= maxLength;
}

bool parseChannel(const std::string& value, uint8_t& channel) {
    if (value.empty() || value.length() > 2) {
        return false;
    }

    char* end = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (end == value.c_str() || *end != '\0' || parsed < 1 ||
        parsed > kMaxWifiChannel) {
        return false;
    }

    channel = static_cast<uint8_t>(parsed);
    return true;
}

std::string createProvisioningNonce() {
#if defined(ESP32)
    const uint32_t value = esp_random();
#elif defined(ESP8266)
    const uint32_t value = os_random();
#else
    const uint32_t value = micros();
#endif
    return String(value, HEX).c_str();
}

const char kProvisionPage[] =
    "<!doctype html><html><head><meta charset=\"utf-8\"><title>Provision "
    "device</title></head><body><h1>First-boot provisioning</h1>"
    "<p>Configure the station network and management credentials.</p>"
    "<form method=\"post\" action=\"/provision\">"
    "<label>STA SSID <input name=\"ssid\" required maxlength=\"32\"></label><br>"
    "<label>STA password <input name=\"password\" type=\"password\" "
    "maxlength=\"63\"> (leave empty for an open network)</label><br>"
    "<label>Network name <input name=\"networkName\" maxlength=\"32\"></label><br>"
    "<label>Channel <input name=\"channel\" type=\"number\" min=\"1\" "
    "max=\"14\" value=\"1\"></label><br>"
     "<input name=\"provision_nonce\" type=\"hidden\" value=\"__PROVISION_NONCE__\">"
     "<label>Management login <input name=\"ota_login\" required "
    "maxlength=\"64\"></label><br>"
    "<label>Management password <input name=\"ota_password\" type=\"password\" "
    "required maxlength=\"64\"></label><br>"
    "<button type=\"submit\">Save</button></form></body></html>";
}  // namespace

//*********************************************************************************************
//!                                     API Server
//*********************************************************************************************

APIServer::APIServer(ProjectConfig& configManager, AsyncServer_t& async_server,
                     AsyncOTA* async_ota)
    : BaseAPI(configManager), async_server(async_server), async_ota(async_ota) {}

APIServer::~APIServer() {
    async_server.removeHandlers(registeredHandlers);
    delete captiveHandler;
    captiveHandler = nullptr;
}
void APIServer::begin() {
    if (started) {
        return;
    }
    started = true;
    provisioningNonce = createProvisioningNonce();
    provisioningNonceUsed = false;

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

    registeredHandlers.push_back(async_server.registerHandler(
        api_url, XHTTP_ANY,
        [this](AsyncWebServerRequest* request) { handleRequest(request); }));

    registeredHandlers.push_back(async_server.registerHandler(
        wifi_manager_url, HTTP_ANY,
        [this](AsyncWebServerRequest* request) { handleRequest(request); }));

    //* Add default JSON handler
    // create JSON route
    std::string json_url = async_server.api_url;
    json_url.append(async_server.json_url);

    registeredHandlers.push_back(async_server.registerHandler(
        json_url.c_str(), XHTTP_GET, [this](AsyncWebServerRequest* request) {
            if (!async_server.authenticate(request)) {
                return;
            }
            request->send(400, MIMETYPE_JSON,
                          "{\"msg\":\"Invalid Request Type\"}");
        }));

    registeredHandlers.push_back(async_server.registerHandler(
        new AsyncCallbackJsonWebHandler(
            json_url.c_str(),
            [this](AsyncWebServerRequest* request, JsonVariant& json) {
                if (!async_server.authenticate(request)) {
                    return;
                }
                handleJson(request, json);
             })));

    registeredHandlers.push_back(async_server.registerHandler(
        "/provision", XHTTP_GET,
        [this](AsyncWebServerRequest* request) { handleProvisionGet(request); }));
    registeredHandlers.push_back(async_server.registerHandler(
        "/provision", XHTTP_POST,
        [this](AsyncWebServerRequest* request) { handleProvisionPost(request); }));

    if (async_ota != nullptr)
        async_ota->begin();
    async_server.server.begin();
}

void APIServer::setupServer() {
    // Set default routes
    routes.reserve(10);  // reserve enough memory for all routes
    routes.emplace("wifi", &APIServer::setWiFi);
    routes.emplace("resetConfig", &APIServer::factoryReset);
    routes.emplace("getDeviceConfigData", &APIServer::getDeviceConfigData);
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

// TODO: Add support for body parsing
void APIServer::handleRequest(AsyncWebServerRequest* request) {
    if (!async_server.authenticate(request)) {
        return;
    }

    std::vector<std::string> temp =
        Helpers::split(async_server.user_commands.c_str(), '/');

    // Guard against a malformed user_commands prefix; skip user-command
    // matching and fall through to normal routing when it cannot match.
    if (temp.size() >= 2 &&
        strcmp(request->pathArg(0).c_str(), temp[1].c_str()) == 0) {
        handle_user_commands(request);
        return;
    }

    // Get the route; path arguments only, never the full URL (query secrets).
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
}

bool APIServer::provisioningAvailable() const {
    const auto& deviceConfig = configManager.getDeviceConfig();
    const auto& apConfig = configManager.getAPWifiConfig();
    const int wifiMode = WiFi.getMode();
    return deviceConfig.ota_login.empty() && deviceConfig.ota_password.empty() &&
           wifiMode == WIFI_AP &&
           validLength(apConfig.password, kMinWpa2PasswordLength,
                       kMaxWifiPasswordLength) &&
           apConfig.password != kLegacyDefaultApPassword;
}

void APIServer::handleProvisionGet(AsyncWebServerRequest* request) {
    if (!provisioningAvailable()) {
        request->send(404, "text/plain", "Provisioning unavailable");
        return;
    }

    std::string page = kProvisionPage;
    const std::string marker = "__PROVISION_NONCE__";
    const size_t markerPosition = page.find(marker);
    if (markerPosition == std::string::npos) {
        request->send(500, "text/plain", "Provisioning unavailable");
        return;
    }
    page.replace(markerPosition, marker.size(), provisioningNonce);
    request->send(200, MIMETYPE_HTML, page.c_str());
}

void APIServer::handleProvisionPost(AsyncWebServerRequest* request) {
    if (provisioningNonceUsed || !provisioningAvailable()) {
        request->send(404, "text/plain", "Provisioning unavailable");
        return;
    }

    if (request->contentLength() > kMaxProvisionBodyBytes ||
        request->params() > 16) {
        request->send(413, "text/plain", "Provisioning request too large");
        return;
    }

    std::string ssid;
    std::string password;
    std::string networkName;
    std::string channelValue;
    std::string otaLogin;
    std::string otaPassword;
    std::string nonce;
    if (!readPostParam(request, "ssid", ssid, true) ||
        !readPostParam(request, "password", password, false) ||
        !readPostParam(request, "networkName", networkName, false) ||
        !readPostParam(request, "channel", channelValue, false) ||
        !readPostParam(request, "ota_login", otaLogin, true) ||
        !readPostParam(request, "ota_password", otaPassword, true) ||
        !readPostParam(request, "provision_nonce", nonce, true) ||
        nonce != provisioningNonce ||
        !validLength(ssid, 1, kMaxSsidLength) ||
        (!password.empty() &&
         !validLength(password, 0, kMaxWifiPasswordLength)) ||
        !validLength(otaLogin, 1, kMaxManagementLoginLength) ||
        !validLength(otaPassword, kMinWpa2PasswordLength,
                     kMaxManagementPasswordLength)) {
        request->send(400, "text/plain", "Invalid provisioning values");
        return;
    }

    if (!networkName.empty() && !validLength(networkName, 1, kMaxSsidLength)) {
        request->send(400, "text/plain", "Invalid provisioning values");
        return;
    }

    uint8_t channel = kDefaultProvisionChannel;
    if (!channelValue.empty() && !parseChannel(channelValue, channel)) {
        request->send(400, "text/plain", "Invalid provisioning values");
        return;
    }
    if (networkName.empty()) {
        networkName = ssid;
    }

    auto& networks = configManager.getWifiConfigs();
    const bool updatesExisting = std::any_of(
        networks.begin(), networks.end(),
        [&ssid](const auto& network) { return network.ssid == ssid; });
    if (!updatesExisting && networks.size() >= 3) {
        request->send(422, "text/plain", "No network configuration capacity");
        return;
    }

    const uint8_t power = configManager.getWifiTxPowerConfig().power;
    configManager.setWifiConfig(networkName, ssid, password, channel, power,
                                false, false);
    configManager.setDeviceConfig(otaLogin, otaPassword,
                                  configManager.getDeviceConfig().ota_port,
                                  false);
    configManager.save();
    if (!configManager.lastSaveSucceeded()) {
        // Persistence failed; drop the just-applied management credentials so
        // first-boot provisioning stays reachable after a restart.
        configManager.setDeviceConfig(std::string(), std::string(),
                                      configManager.getDeviceConfig().ota_port,
                                      false);
        if (!updatesExisting) {
            // Roll back a newly added network too; leave updated existing
            // networks in place.
            configManager.deleteWifiConfig(networkName, false);
        }
        request->send(500, "text/plain", "Provisioning save failed");
        return;
    }

    provisioningNonceUsed = true;
    request->send(202, "text/plain", "Provisioning accepted; restart device");
}

void APIServer::addAPICommand(const std::string& url,
                              ArRequestHandlerFunction funct) {
    useRouteHandler.emplace(std::move(url), funct);
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
void APIServer::handle_user_commands(AsyncWebServerRequest* request) {
    std::string url = request->pathArg(1).c_str();
    auto it = useRouteHandler.find(url);
    if (it != useRouteHandler.end()) {
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
    delete captiveHandler;
    auto* handler = new CaptiveRequestHandler(async_server);
    handler->setFilter(apMode ? ON_AP_FILTER : ON_STA_FILTER);
    captiveHandler = handler;
    registeredHandlers.push_back(async_server.registerHandler(handler));
}

//*********************************************************************************************
//!                                     API Utilities
//*********************************************************************************************

CaptiveRequestHandler::CaptiveRequestHandler(AsyncServer_t& async_server)
    : async_server(async_server) {}

CaptiveRequestHandler::~CaptiveRequestHandler() {}

bool CaptiveRequestHandler::canHandle(AsyncWebServerRequest* request) {
    const String& url = request->url();
    return url != "/provision" && !url.startsWith("/api") &&
           !url.startsWith("/update");
}
void CaptiveRequestHandler::handleRequest(AsyncWebServerRequest* request) {
    if (!async_server.authenticate(request)) {
        return;
    }

    if (async_server.spiffsMounted) {
        if (async_server.custom_html_files.size() > 0) {
            for (auto& file : async_server.custom_html_files) {
                if (file.endpoint == "captive_portal" ||
                    file.endpoint == "captive_portal.html" ||
                    file.endpoint == "/" || file.endpoint == "/index.html" ||
                    file.endpoint == "/index" ||
                    file.endpoint == "portal.html") {
                    request->send(async_server.webFilesystem(),
                                  file.file.c_str(),
                                  API_Utilities::MIMETYPE_HTML);
                    return;
                }
            }
            // No captive-specific custom file; fall through to the built-in
            // page below.
        }
    }

    AsyncWebServerResponse* response = request->beginResponse(
        200, "text/html", WEB_MANAGER_HTML, WEB_MANAGER_HTML_SIZE);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
}
