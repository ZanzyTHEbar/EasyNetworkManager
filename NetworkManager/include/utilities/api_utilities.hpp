#pragma once

#include <SPIFFS.h>

#include <string>

#include "mbedtls/md.h"
#include "utilities/network_utilities.hpp"

constexpr int XHTTP_GET = 0b00000001;
constexpr int XHTTP_POST = 0b00000010;
constexpr int XHTTP_DELETE = 0b00000100;
constexpr int XHTTP_PUT = 0b00001000;
constexpr int XHTTP_PATCH = 0b00010000;
constexpr int XHTTP_HEAD = 0b00100000;
constexpr int XHTTP_OPTIONS = 0b01000000;
constexpr int XHTTP_ANY = 0b01111111;

#include <ESPAsyncWebServer.h>

class API_Utilities {
   public:
    API_Utilities();
    virtual ~API_Utilities();

    std::string readFile(fs::FS& fs, const std::string& path);
    void writeFile(fs::FS& fs, const std::string& path,
                   const std::string& message);
    std::string shaEncoder(const std::string& data);

   protected:
    bool initSPIFFS();

    typedef std::unordered_map<std::string, WebRequestMethodComposite>
        networkMethodsMap_t;

   public:
    std::unordered_map<std::string, WebRequestMethodComposite>
        _networkMethodsMap_inv = {
            {"GET", XHTTP_GET},     {"POST", XHTTP_POST},
            {"PUT", XHTTP_PUT},     {"DELETE", XHTTP_DELETE},
            {"PATCH", XHTTP_PATCH}, {"OPTIONS", XHTTP_OPTIONS},
    };

    std::unordered_map<WebRequestMethodComposite, std::string>
        _networkMethodsMap = {
            {XHTTP_GET, "GET"},     {XHTTP_POST, "POST"},
            {XHTTP_PUT, "PUT"},     {XHTTP_DELETE, "DELETE"},
            {XHTTP_PATCH, "PATCH"}, {XHTTP_OPTIONS, "OPTIONS"},
    };

    enum RequestMethods { GET, POST, PUT, DELETE, PATCH, OPTIONS };

    std::unordered_map<WebRequestMethodComposite, RequestMethods>
        _networkMethodsMap_enum = {
            {XHTTP_GET, GET},     {XHTTP_POST, POST},
            {XHTTP_PUT, PUT},     {XHTTP_DELETE, DELETE},
            {XHTTP_PATCH, PATCH}, {XHTTP_OPTIONS, OPTIONS},
    };

   public:
    static const char* MIMETYPE_HTML;
    static const char* MIMETYPE_CSS;
    static const char* MIMETYPE_JS;
    static const char* MIMETYPE_PNG;
    static const char* MIMETYPE_JPG;
    static const char* MIMETYPE_ICO;
    static const char* MIMETYPE_JSON;
};
