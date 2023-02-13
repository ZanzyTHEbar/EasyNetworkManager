#ifndef APIUTILITIES_HPP
#define APIUTILITIES_HPP

#include <SPIFFS.h>

#include <string>

#include "mbedtls/md.h"
#include "utilities/network_utilities.hpp"

class API_Utilities {
   public:
    API_Utilities();
    virtual ~API_Utilities();

   protected:
    std::string readFile(fs::FS& fs, std::string path);
    void writeFile(fs::FS& fs, std::string path, std::string message);
    bool initSPIFFS();
    std::string shaEncoder(std::string data);

   protected:
    static const char* MIMETYPE_HTML;
    /* static const char *MIMETYPE_CSS; */
    /* static const char *MIMETYPE_JS; */
    /* static const char *MIMETYPE_PNG; */
    /* static const char *MIMETYPE_JPG; */
    /* static const char *MIMETYPE_ICO; */
    static const char* MIMETYPE_JSON;
};

#endif  // APIUTILITIES_HPP