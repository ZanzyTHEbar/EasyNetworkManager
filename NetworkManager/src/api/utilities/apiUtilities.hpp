#ifndef APIUTILITIES_HPP
#define APIUTILITIES_HPP

#include <unordered_map>
#include <string>
#include <sstream>

#define WEBSERVER_H

constexpr int HTTP_GET = 0b00000001;
constexpr int HTTP_POST = 0b00000010;
constexpr int HTTP_DELETE = 0b00000100;
constexpr int HTTP_PUT = 0b00001000;
constexpr int HTTP_PATCH = 0b00010000;
constexpr int HTTP_HEAD = 0b00100000;
constexpr int HTTP_OPTIONS = 0b01000000;
constexpr int HTTP_ANY = 0b01111111;

#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include "mbedtls/md.h"
#include "wifihandler/utilities/utilities.hpp"

class API_Utilities
{
public:
    API_Utilities(int CONTROL_PORT,
                  WiFiHandler *network,
                  DNSServer *dnsServer,
                  std::string api_url,
                  std::string wifimanager_url,
                  std::string userCommands);
    virtual ~API_Utilities();

protected:
    void notFound(AsyncWebServerRequest *request) const;
    String readFile(fs::FS &fs, std::string path);
    void writeFile(fs::FS &fs, std::string path, std::string message);
    std::vector<std::string> split(const std::string &s, char delimiter);
    bool initSPIFFS();
    std::string shaEncoder(std::string data);
    std::unordered_map<WebRequestMethodComposite, std::string> _networkMethodsMap = {
        {HTTP_GET, "GET"},
        {HTTP_POST, "POST"},
        {HTTP_PUT, "PUT"},
        {HTTP_DELETE, "DELETE"},
        {HTTP_PATCH, "PATCH"},
        {HTTP_OPTIONS, "OPTIONS"},
    };

    enum RequestMethods
    {
        GET,
        POST,
        PUT,
        DELETE,
        PATCH,
        OPTIONS
    };

    std::unordered_map<WebRequestMethodComposite, RequestMethods> _networkMethodsMap_enum = {
        {HTTP_GET, GET},
        {HTTP_POST, POST},
        {HTTP_PUT, PUT},
        {HTTP_DELETE, DELETE},
        {HTTP_PATCH, PATCH},
        {HTTP_OPTIONS, OPTIONS},
    };

protected:
    AsyncWebServer *server;
    WiFiHandler *network;
    DNSServer *dnsServer;
    typedef std::unordered_map<std::string, WebRequestMethodComposite> networkMethodsMap_t;

protected:
    std::string api_url;
    std::string wifimanager_url;
    std::string userCommands;

    static bool ssid_write;
    static bool pass_write;
    static bool channel_write;

    static const char *MIMETYPE_HTML;
    /* static const char *MIMETYPE_CSS; */
    /* static const char *MIMETYPE_JS; */
    /* static const char *MIMETYPE_PNG; */
    /* static const char *MIMETYPE_JPG; */
    /* static const char *MIMETYPE_ICO; */
    static const char *MIMETYPE_JSON;
};

#endif // APIUTILITIES_HPP