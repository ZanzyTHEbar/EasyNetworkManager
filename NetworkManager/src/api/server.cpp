#include "api/server.hpp"

AsyncServer_t::AsyncServer_t(const int CONTROL_PORT,
                             ProjectConfig& configManager,
                             const std::string& api_url,
                             const std::string& wifimanager_url,
                             const std::string& userCommands)
    : server(CONTROL_PORT),
      configManager(configManager),
      api_url(std::move(api_url)),
      wifimanager_url(std::move(wifimanager_url)),
      userCommands(std::move(userCommands)),
      spiffsMounted(false) {
    spiffsMounted = initSPIFFS();
}
AsyncServer_t::~AsyncServer_t() {}

void AsyncServer_t::begin() {
    server.on("/", XHTTP_GET,
              [&](AsyncWebServerRequest* request) { request->send(200); });

#ifdef USE_WEBMANAGER
    server.on(wifimanager_url.c_str(), XHTTP_GET,
              [&](AsyncWebServerRequest* request) {
                  // TODO: add authentication support
                  /* if (_authRequired) {
                      if (!request->authenticate(_username.c_str(),
                                                 _password.c_str())) {
                          return request->requestAuthentication();
                      }
                  } */
                  AsyncWebServerResponse* response = request->beginResponse_P(
                      200, "text/html", WEB_MANAGER_HTML,
                      WEB_MANAGER_HTML_SIZE);
                  response->addHeader("Content-Encoding", "gzip");
                  request->send(response);
              });
#endif  // USE_WEBManager

    // preflight cors check
    server.on("/", XHTTP_OPTIONS, [&](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse(204);
        response->addHeader("Access-Control-Allow-Methods",
                            "PUT,POST,GET,OPTIONS");
        response->addHeader("Access-Control-Allow-Headers",
                            "Accept, Content-Type, Authorization, FileSize");
        response->addHeader("Access-Control-Allow-Credentials", "true");
        request->send(response);
    });

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

    server.onNotFound(
        [&](AsyncWebServerRequest* request) { notFound(request); });

    if (!spiffsMounted && custom_html_files.size() <= 0) {
        log_e(
            "SPIFFS not initialized - no user defined html files available. "
            "API will still function, no custom html files have been loaded. "
            "\n");
        return;
    }

    for (auto& route : custom_html_files) {
        server.on(route.endpoint.c_str(), _networkMethodsMap_inv[route.method],
                  [&](AsyncWebServerRequest* request) {
                      // TODO: add authentication support
                      /* if (_authRequired) {
                          if (!request->authenticate(_username.c_str(),
                                                     _password.c_str()))
                      { return request->requestAuthentication();
                          }
                      } */
                      request->send(SPIFFS, route.file.c_str(), MIMETYPE_HTML);
                  });
    }
    /* server.serveStatic(wifimanager_url.c_str(), SPIFFS,
       "/wifimanager.html") .setCacheControl("max-age=600"); */
}

void AsyncServer_t::notFound(AsyncWebServerRequest* request) const {
    if (_networkMethodsMap.find(request->method()) !=
        _networkMethodsMap.end()) {
        log_i("%s: http://%s%s/\n",
              _networkMethodsMap.at(request->method()).c_str(),
              request->host().c_str(), request->url().c_str());
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "Request %s Not found: %s",
                 _networkMethodsMap.at(request->method()).c_str(),
                 request->url().c_str());
        request->send(404, "text/plain", buffer);
    } else {
        request->send(404, "text/plain",
                      "Request Not found using unknown method");
    }
}
