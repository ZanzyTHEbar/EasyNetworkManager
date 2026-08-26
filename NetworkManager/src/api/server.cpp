#include <api/server.hpp>

AsyncServer_t::AsyncServer_t(const int CONTROL_PORT,
                             ProjectConfig& configManager,
                             const std::string& api_url,
                             const std::string& wifimanager_url,
                             const std::string& user_commands,
                             const std::string& json_url)
    : server(CONTROL_PORT),
      configManager(configManager),
      api_url(std::move(api_url)),
      wifimanager_url(std::move(wifimanager_url)),
      user_commands(std::move(user_commands)),
      json_url(std::move(json_url)),
      spiffsMounted(false),
      _started(false) {}
AsyncServer_t::~AsyncServer_t() {
    server.end();
    server.reset();
}

bool AsyncServer_t::authenticate(AsyncWebServerRequest* request) const {
    const auto& device_config = configManager.getDeviceConfig();
    return authenticate(request, device_config.ota_login.c_str(),
                        device_config.ota_password.c_str());
}

bool AsyncServer_t::authenticate(AsyncWebServerRequest* request,
                                 const char* login,
                                 const char* password) const {
    if (login == nullptr || password == nullptr || login[0] == '\0' ||
        password[0] == '\0') {
        request->requestAuthentication(nullptr, false);
        return false;
    }

    if (!allowsInsecureHttp()) {
        request->send(503, "text/plain", "Secure transport required");
        return false;
    }

    if (!request->authenticate(login, password, nullptr, false)) {
        request->requestAuthentication(nullptr, false);
        return false;
    }

    return true;
}

AsyncWebHandler* AsyncServer_t::registerHandler(AsyncWebHandler* handler) {
    return &server.addHandler(handler);
}

AsyncWebHandler* AsyncServer_t::registerHandler(
    const char* uri, WebRequestMethodComposite method,
    ArRequestHandlerFunction onRequest, ArUploadHandlerFunction onUpload,
    ArBodyHandlerFunction onBody) {
    return &server.on(uri, method, std::move(onRequest), std::move(onUpload),
                       std::move(onBody));
}

void AsyncServer_t::removeHandlers(
    std::vector<AsyncWebHandler*>& handlers) {
    for (AsyncWebHandler* handler : handlers) {
        if (handler != nullptr) {
            server.removeHandler(handler);
        }
    }
    handlers.clear();
}

void AsyncServer_t::begin() {
    if (_started) {
        return;
    }
    _started = true;

    spiffsMounted = initSPIFFS();

    server.on("/", XHTTP_GET, [](AsyncWebServerRequest* request) {
        if (!AsyncServer_t::allowsInsecureHttp()) {
            request->send(503, "text/plain", "Secure transport required");
            return;
        }
        request->send(200);
    });

#ifdef USE_WEBMANAGER
    server.on(wifimanager_url.c_str(), XHTTP_GET,
              [this](AsyncWebServerRequest* request) {
                  if (!authenticate(request)) {
                      return;
                  }
                  AsyncWebServerResponse* response = request->beginResponse(
                      200, "text/html", WEB_MANAGER_HTML,
                      WEB_MANAGER_HTML_SIZE);
                  response->addHeader("Content-Encoding", "gzip");
                  request->send(response);
              });
#endif  // USE_WEBManager

    server.onNotFound([this](AsyncWebServerRequest* request) {
        notFound(request);
    });

    if (!spiffsMounted && custom_html_files.size() <= 0) {
        log_e(
            "Web filesystem not initialized - no user defined html files available. "
            "API will still function, no custom html files have been loaded. "
            "\n");
        return;
    }

    for (const auto& route : custom_html_files) {
        const auto method = _networkMethodsMap_inv.find(route.method);
        if (method == _networkMethodsMap_inv.end()) {
            log_e("Invalid custom route method: %s", route.method.c_str());
            continue;
        }

        const std::string endpoint = route.endpoint;
        const std::string file = route.file;
        server.on(endpoint.c_str(), method->second,
                  [this, file](AsyncWebServerRequest* request) {
                      if (!authenticate(request)) {
                          return;
                      }
                      request->send(webFilesystem(), file.c_str(), MIMETYPE_HTML);
                  });
    }
    /* server.serveStatic(wifimanager_url.c_str(), webFilesystem(),
       "/wifimanager.html") .setCacheControl("max-age=600"); */
}

void AsyncServer_t::notFound(AsyncWebServerRequest* request) const {
    if (_networkMethodsMap.find(request->method()) !=
        _networkMethodsMap.end()) {
        String safeUrl = request->url();
        const int queryStart = safeUrl.indexOf('?');
        if (queryStart >= 0) {
            safeUrl.remove(queryStart);
        }
        log_i("%s: http://%s%s/\n",
              _networkMethodsMap.at(request->method()).c_str(),
              request->host().c_str(), safeUrl.c_str());
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "Request %s Not found: %s",
                 _networkMethodsMap.at(request->method()).c_str(),
                 safeUrl.c_str());
        request->send(404, "text/plain", buffer);
    } else {
        request->send(404, "text/plain",
                      "Request Not found using unknown method");
    }
}
