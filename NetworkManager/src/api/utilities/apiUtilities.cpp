#include "apiUtilities.hpp"

//! These have to be called before the constructor of the class because they are static
//! C++ 11 does not have inline variables, sadly. So we have to do this.
const char *API_Utilities::MIMETYPE_HTML{"text/html"};
// const char *BaseAPI::MIMETYPE_CSS{"text/css"};
// const char *BaseAPI::MIMETYPE_JS{"application/javascript"};
// const char *BaseAPI::MIMETYPE_PNG{"image/png"};
// const char *BaseAPI::MIMETYPE_JPG{"image/jpeg"};
// const char *BaseAPI::MIMETYPE_ICO{"image/x-icon"};
const char *API_Utilities::MIMETYPE_JSON{"application/json"};

bool API_Utilities::ssid_write = false;
bool API_Utilities::pass_write = false;
bool API_Utilities::channel_write = false;

//*********************************************************************************************
//!                                     API Utilities
//*********************************************************************************************

API_Utilities::API_Utilities(int CONTROL_PORT,
                             WiFiHandler *network,
                             DNSServer *dnsServer,
                             const std::string &api_url,
                             const std::string &wifimanager_url,
                             const std::string &userCommands) : server(new AsyncWebServer(CONTROL_PORT)),
                                                                dnsServer(NULL),
                                                                network(std::move(network)),
                                                                api_url(std::move(api_url)),
                                                                wifimanager_url(std::move(wifimanager_url)),
                                                                userCommands(std::move(userCommands))
{
    if (dnsServer != NULL)
    {
        this->dnsServer = dnsServer;
    }
}
API_Utilities::~API_Utilities() {}
std::string API_Utilities::shaEncoder(std::string data)
{
    const char *data_c = data.c_str();
    int size = 64;
    uint8_t hash[size];
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA512;

    const size_t len = strlen(data_c);
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const unsigned char *)data_c, len);
    mbedtls_md_finish(&ctx, hash);
    mbedtls_md_free(&ctx);

    std::string hash_string = "";
    for (uint16_t i = 0; i < size; i++)
    {
        std::string hex = String(hash[i], HEX).c_str();
        if (hex.length() < 2)
        {
            hex = "0" + hex;
        }
        hash_string += hex;
    }
    return hash_string;
}

void API_Utilities::notFound(AsyncWebServerRequest *request) const
{
    if (_networkMethodsMap.find(request->method()) != _networkMethodsMap.end())
    {
        log_i("%s: http://%s%s/\n", _networkMethodsMap.at(request->method()).c_str(), request->host().c_str(), request->url().c_str());
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "Request %s Not found: %s", _networkMethodsMap.at(request->method()).c_str(), request->url().c_str());
        request->send(404, "text/plain", buffer);
    }
    else
    {
        request->send(404, "text/plain", "Request Not found using unknown method");
    }
}

// Initialize SPIFFS
bool API_Utilities::initSPIFFS()
{
    bool init_spiffs = SPIFFS.begin(false);
    log_e("[SPIFFS]: SPIFFS Initialized: %s", init_spiffs ? "true" : "false");
    return init_spiffs;
}

// Read File from SPIFFS
String API_Utilities::readFile(fs::FS &fs, std::string path)
{
    log_i("Reading file: %s\r\n", path.c_str());

    File file = fs.open(path.c_str());
    if (!file || file.isDirectory())
    {
        log_e("[INFO]: Failed to open file for reading");
        return String();
    }

    String fileContent;
    while (file.available())
    {
        fileContent = file.readStringUntil('\n');
        break;
    }
    return fileContent;
}

// Write file to SPIFFS
void API_Utilities::writeFile(fs::FS &fs, std::string path, std::string message)
{
    log_i("[Writing File]: Writing file: %s\r\n", path);
    Network_Utilities::my_delay(0.1L);

    File file = fs.open(path.c_str(), FILE_WRITE);
    if (!file)
    {
        log_i("[Writing File]: failed to open file for writing");
        return;
    }
    if (file.print(message.c_str()))
    {
        log_i("[Writing File]: file written");
    }
    else
    {
        log_i("[Writing File]: file write failed");
    }
}

std::vector<std::string> API_Utilities::split(const std::string &s, char delimiter)
{
    std::vector<std::string> parts;
    std::string part;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, part, delimiter))
    {
        parts.push_back(part);
    }
    return parts;
}
