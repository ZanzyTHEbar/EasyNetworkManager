#pragma once

#include <data/config/project_config.hpp>
#include <utilities/api_utilities.hpp>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include "server.hpp"

#if defined(ESP32)
#    include <mbedtls/sha256.h>
#    include <unordered_map>
#endif

using AsyncOTACustomHandlerFunction = std::function<void(void)>;

// AsyncServer_t and ProjectConfig must outlive AsyncOTA. Handler removal is
// intended for normal teardown, not concurrent destruction during a request.
class AsyncOTA {
    AsyncOTACustomHandlerFunction customHandlerFunction = NULL;
    AsyncServer_t& async_server;
    std::string publicKeyPem;

#if defined(ESP32)
    struct UploadState {
        mbedtls_sha256_context sha256;
        std::vector<uint8_t> signature;
        std::string publicKeyPem;
        bool hashStarted = false;

        UploadState() { mbedtls_sha256_init(&sha256); }
        ~UploadState() { mbedtls_sha256_free(&sha256); }
        UploadState(const UploadState&) = delete;
        UploadState& operator=(const UploadState&) = delete;
    };

    std::unordered_map<AsyncWebServerRequest*, UploadState> uploadStates;
#endif

    AsyncWebServerRequest* ownedUploadRequest = nullptr;

    void abortUpload(AsyncWebServerRequest* request);
    bool claimUpload(AsyncWebServerRequest* request);
    void releaseUpload(AsyncWebServerRequest* request);
    void handleUploadDisconnect(AsyncWebServerRequest* request);
    void markUploadError(AsyncWebServerRequest* request, const char* message,
                         int status);

   protected:
    ProjectConfig& configManager;
    std::vector<AsyncWebHandler*> registeredHandlers;
    bool started = false;

   public:
    AsyncOTA(ProjectConfig& configManager, AsyncServer_t& async_server);
    virtual ~AsyncOTA();
    void begin();
    bool checkAuthentication(AsyncWebServerRequest* request);
    bool checkAuthentication(AsyncWebServerRequest* request, const char* login,
                             const char* password);
    void setOTAHandler(AsyncOTACustomHandlerFunction customHandlerFunction);

    // Copies a PEM public key. Private key material is neither accepted nor
    // stored. Returns false when the key is empty, invalid, or unsupported.
    bool setPublicKey(const std::string& publicKeyPem);
    void clearPublicKey();
    bool hasPublicKey() const;

    // Kept for source compatibility; AsyncOTA authentication is mandatory.
    bool _authRequired;
};
