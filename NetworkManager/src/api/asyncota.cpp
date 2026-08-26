#include <FS.h>
#if defined(ESP8266)
#    include <Updater.h>
#else
#include <Update.h>
#endif
#include <atomic>
#include <utility>

#if defined(ESP32)
#    include <mbedtls/base64.h>
#    include <mbedtls/pk.h>
#endif

#include "api/asyncota.hpp"
#include "api/elegantWebpage.h"

namespace {
constexpr const char* kOtaSignature = "signature";
constexpr const char* kOtaSignatureHeader = "X-OTA-Signature";
constexpr const char* kOtaMd5 = "MD5";
constexpr const char* kOtaMd5Header = "X-OTA-MD5";
constexpr const char* kOtaUploadComplete = "ota_upload_complete";
constexpr const char* kOtaUploadError = "ota_upload_error";
constexpr const char* kOtaResponseSent = "ota_response_sent";
constexpr const char* kOtaUpdateStarted = "ota_update_started";
constexpr const char* kOtaUploadStatus = "ota_upload_status";

// Arduino Update is a process-global singleton, so this guard must be shared
// by every AsyncOTA instance.
std::atomic<AsyncWebServerRequest*> activeUploadRequest{nullptr};

bool authenticateCurrentConfig(AsyncServer_t& async_server,
                               ProjectConfig& config_manager,
                               AsyncWebServerRequest* request) {
    const auto& device_config = config_manager.getDeviceConfig();
    return async_server.authenticate(request, device_config.ota_login.c_str(),
                                     device_config.ota_password.c_str());
}

bool readUniqueQueryOrHeader(AsyncWebServerRequest* request,
                             const char* queryName, const char* headerName,
                             String& value) {
    const AsyncWebParameter* queryParameter = nullptr;
    size_t queryMatches = 0;
    for (size_t i = 0; i < request->params(); ++i) {
        const AsyncWebParameter* parameter = request->getParam(i);
        if (parameter != nullptr && !parameter->isPost() &&
            !parameter->isFile() && parameter->name() == queryName) {
            queryParameter = parameter;
            ++queryMatches;
        }
    }

    const AsyncWebHeader* header = nullptr;
    size_t headerMatches = 0;
    for (size_t i = 0; i < request->headers(); ++i) {
        const AsyncWebHeader* candidate = request->getHeader(i);
        if (candidate != nullptr && candidate->name().equalsIgnoreCase(headerName)) {
            header = candidate;
            ++headerMatches;
        }
    }

    if ((queryMatches != 1 && headerMatches != 1) ||
        (queryMatches == 1 && headerMatches == 1)) {
        return false;
    }

    value = queryMatches == 1 ? queryParameter->value() : header->value();
    return !value.isEmpty();
}

#if defined(ESP32)
constexpr size_t kMaxSignatureBytes = 4096;
constexpr size_t kMaxSignatureEncodedBytes =
    ((kMaxSignatureBytes + 2) / 3) * 4;

bool parsePublicKey(const std::string& pem, mbedtls_pk_context& key) {
    if (mbedtls_pk_parse_public_key(
            &key, reinterpret_cast<const unsigned char*>(pem.c_str()),
            pem.size() + 1) != 0) {
        return false;
    }

    const mbedtls_pk_type_t type = mbedtls_pk_get_type(&key);
    return type == MBEDTLS_PK_RSA || type == MBEDTLS_PK_ECKEY ||
           type == MBEDTLS_PK_ECDSA;
}

bool decodeSignature(const String& encoded, std::vector<uint8_t>& signature) {
    const size_t encodedLength = encoded.length();
    if (encodedLength == 0 || encodedLength > kMaxSignatureEncodedBytes) {
        return false;
    }

    std::vector<uint8_t> decoded((encodedLength / 4) * 3 + 3);
    size_t decodedLength = 0;
    if (mbedtls_base64_decode(
            decoded.data(), decoded.size(), &decodedLength,
            reinterpret_cast<const unsigned char*>(encoded.c_str()),
            encodedLength) != 0 ||
        decodedLength == 0 || decodedLength > kMaxSignatureBytes) {
        return false;
    }

    decoded.resize(decodedLength);
    signature = std::move(decoded);
    return true;
}

bool readDetachedSignature(AsyncWebServerRequest* request,
                           std::vector<uint8_t>& signature) {
    String encoded;
    if (!readUniqueQueryOrHeader(request, kOtaSignature, kOtaSignatureHeader,
                                 encoded)) {
        return false;
    }
    return decodeSignature(encoded, signature);
}

bool verifySignature(const std::string& pem,
                     const std::vector<uint8_t>& signature,
                     const uint8_t digest[32]) {
    mbedtls_pk_context key;
    mbedtls_pk_init(&key);
    const bool parsed = parsePublicKey(pem, key);
    const bool verified =
        parsed &&
        mbedtls_pk_verify(&key, MBEDTLS_MD_SHA256, digest, 32,
                          signature.data(), signature.size()) == 0;
    mbedtls_pk_free(&key);
    return verified;
}
#endif

int filesystemUpdateCommand() {
#if defined(ESP8266)
    return U_FS;
#else
    return U_SPIFFS;
#endif
}

size_t updateSize() {
#if defined(ESP8266)
    const size_t freeSpace = ESP.getFreeSketchSpace();
    return freeSpace > 0x1000 ? (freeSpace - 0x1000) & ~size_t(0xFFF) : 0;
#else
    return UPDATE_SIZE_UNKNOWN;
#endif
}
}  // namespace

AsyncOTA::AsyncOTA(ProjectConfig& configManager, AsyncServer_t& async_server)
    : configManager(configManager),
      async_server(async_server),
      _authRequired(true) {}

AsyncOTA::~AsyncOTA() {
    AsyncWebServerRequest* request = ownedUploadRequest;
    if (request != nullptr) {
        abortUpload(request);
        releaseUpload(request);
    }
    async_server.removeHandlers(registeredHandlers);
}

bool AsyncOTA::checkAuthentication(AsyncWebServerRequest* request) {
    return authenticateCurrentConfig(async_server, configManager, request);
}

bool AsyncOTA::checkAuthentication(AsyncWebServerRequest* request,
                                   const char* login, const char* password) {
    // Keep the legacy signature, but always use the current configuration.
    (void)login;
    (void)password;
    return checkAuthentication(request);
}

bool AsyncOTA::setPublicKey(const std::string& publicKeyPem) {
#if defined(ESP32)
    if (publicKeyPem.empty() ||
        publicKeyPem.find("PRIVATE KEY") != std::string::npos) {
        return false;
    }

    mbedtls_pk_context key;
    mbedtls_pk_init(&key);
    const bool valid = parsePublicKey(publicKeyPem, key);
    mbedtls_pk_free(&key);
    if (!valid) {
        return false;
    }

    this->publicKeyPem = publicKeyPem;
    return true;
#else
    (void)publicKeyPem;
    return false;
#endif
}

void AsyncOTA::clearPublicKey() {
    publicKeyPem.clear();
}

bool AsyncOTA::hasPublicKey() const {
#if defined(ESP32)
    return !publicKeyPem.empty();
#else
    return false;
#endif
}

void AsyncOTA::abortUpload(AsyncWebServerRequest* request) {
    if (activeUploadRequest.load() == request &&
        request->getAttribute(kOtaUpdateStarted, false)) {
#if defined(ESP32)
        Update.abort();
#elif defined(ESP8266)
        // ESP8266 has no public abort API; end an incomplete update without
        // allowing it to become bootable.
        if (Update.isRunning()) {
            (void)Update.end(false);
        }
#endif
        request->setAttribute(kOtaUpdateStarted, false);
    }
}

bool AsyncOTA::claimUpload(AsyncWebServerRequest* request) {
    AsyncWebServerRequest* noActiveUpload = nullptr;
    if (!activeUploadRequest.compare_exchange_strong(noActiveUpload, request)) {
        return false;
    }

    ownedUploadRequest = request;
#if defined(ESP32)
    if (!uploadStates.try_emplace(request).second) {
        AsyncWebServerRequest* expected = request;
        activeUploadRequest.compare_exchange_strong(expected, noActiveUpload);
        ownedUploadRequest = nullptr;
        return false;
    }
#endif
    return true;
}

void AsyncOTA::releaseUpload(AsyncWebServerRequest* request) {
#if defined(ESP32)
    uploadStates.erase(request);
#endif
    if (ownedUploadRequest == request) {
        ownedUploadRequest = nullptr;
    }
    AsyncWebServerRequest* expected = request;
    activeUploadRequest.compare_exchange_strong(expected, nullptr);
}

void AsyncOTA::handleUploadDisconnect(AsyncWebServerRequest* request) {
    // The client is gone; abort and release only. The POST path owns responses.
    if (activeUploadRequest.load() == request) {
        markUploadError(request, "OTA upload disconnected", 499);
    }
}

void AsyncOTA::markUploadError(AsyncWebServerRequest* request,
                               const char* message, int status) {
    abortUpload(request);
    releaseUpload(request);
    request->setAttribute(kOtaUploadError, message);
    request->setAttribute(kOtaUploadStatus, static_cast<long>(status));
}

void AsyncOTA::begin() {
    if (started) {
        return;
    }

#if !defined(EASYNETWORKMANAGER_ALLOW_INSECURE_OTA)
#    if !defined(ESP32)
    log_e("Signed Async OTA is unavailable on ESP8266; OTA is disabled");
    return;
#    endif
    if (!hasPublicKey()) {
        log_e("Signed Async OTA requires a configured public key; call "
              "setPublicKey before begin");
        return;
    }
#endif

    auto device_config = configManager.getDeviceConfig();
    auto mdns_config = configManager.getMDNSConfig();

    if (device_config.ota_login.empty() || device_config.ota_password.empty()) {
        log_e(
            "OTA login and password are required in order to setup the OTA "
            "server");
        return;
    }

    started = true;

    log_i("[OTA Server]: Initializing OTA Server");
    log_i(
        "[OTA Server]: Navigate to http://%s.local:81/update to update the "
        "firmware",
        mdns_config.hostname.c_str());

    const AsyncOTACustomHandlerFunction custom_handler =
        customHandlerFunction;
    AsyncServer_t* server = &async_server;
    ProjectConfig* config = &configManager;

    registeredHandlers.push_back(async_server.registerHandler(
        "/update/identity", 0b00000001,
        [server, config](AsyncWebServerRequest* request) {
            if (!authenticateCurrentConfig(*server, *config, request)) {
                return;
            }

#if defined(ESP32)
            String id = String((uint32_t)ESP.getEfuseMac(), HEX);
            const char* hardware = "ESP32";
#else
            String id = String(ESP.getChipId(), HEX);
            const char* hardware = "ESP8266";
#endif
            id.toUpperCase();
            request->send(200, API_Utilities::MIMETYPE_JSON,
                          "{\"id\": \"" + id + "\", \"hardware\": \"" +
                              hardware + "\"}");
        }));

    registeredHandlers.push_back(async_server.registerHandler(
        "/update", 0b00000001,
        [server, config, custom_handler](AsyncWebServerRequest* request) {
            if (!authenticateCurrentConfig(*server, *config, request)) {
                return;
            }

            if (custom_handler) {
                custom_handler();
            }

            AsyncWebServerResponse* response = request->beginResponse(
                200, "text/html", ELEGANT_HTML, ELEGANT_HTML_SIZE);
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        }));

    registeredHandlers.push_back(async_server.registerHandler(
        "/update", 0b00000010,
        [this, server, config](AsyncWebServerRequest* request) {
            if (request->hasAttribute(kOtaResponseSent)) {
                releaseUpload(request);
                return;
            }
            if (!authenticateCurrentConfig(*server, *config, request)) {
                abortUpload(request);
                releaseUpload(request);
                request->setAttribute(kOtaResponseSent, true);
                return;
            }

            if (request->hasAttribute(kOtaUploadError)) {
                request->setAttribute(kOtaResponseSent, true);
                request->send(
                    static_cast<int>(
                        request->getAttribute(kOtaUploadStatus, 500L)),
                    "text/plain", request->getAttribute(kOtaUploadError));
                releaseUpload(request);
                return;
            }

            if (!request->hasAttribute(kOtaUploadComplete) ||
                Update.hasError()) {
                request->setAttribute(kOtaResponseSent, true);
                request->send(500, "text/plain", "FAIL");
                releaseUpload(request);
                return;
            }

            AsyncWebServerResponse* response = request->beginResponse(
                200, "text/plain", "OK");
            response->addHeader("Connection", "close");
            request->setAttribute(kOtaResponseSent, true);
            request->send(response);
            releaseUpload(request);
        },
        [this, server, config](AsyncWebServerRequest* request, String filename,
                               size_t index, uint8_t* data, size_t len,
                               bool final) {
            if (request->hasAttribute(kOtaResponseSent) ||
                request->hasAttribute(kOtaUploadError)) {
                return;
            }
            if (!authenticateCurrentConfig(*server, *config, request)) {
                // Store the failure only; the POST callback owns the single
                // terminal response.
                markUploadError(request, "OTA authentication failed", 401);
                return;
            }

            if (!index) {
                request->onDisconnect([this, request]() {
                    handleUploadDisconnect(request);
                });
                if (!claimUpload(request)) {
                    markUploadError(request, "OTA upload already in progress",
                                    409);
                    return;
                }

#if defined(ESP32)
#    if !defined(EASYNETWORKMANAGER_ALLOW_INSECURE_OTA)
                UploadState& state = uploadStates.at(request);
                state.publicKeyPem = publicKeyPem;
                if (!readDetachedSignature(request, state.signature)) {
                    markUploadError(request,
                                    "Detached signature missing or invalid",
                                    400);
                    return;
                }
                if (mbedtls_sha256_starts_ret(&state.sha256, 0) != 0) {
                    markUploadError(request, "OTA hash initialization failed",
                                    500);
                    return;
                }
                state.hashStarted = true;
#    endif
#endif

                String expectedMd5;
                if (!readUniqueQueryOrHeader(request, kOtaMd5, kOtaMd5Header,
                                             expectedMd5)) {
                    markUploadError(request,
                                    "MD5 must be supplied exactly once as a "
                                    "query parameter or header",
                                    400);
                    return;
                }

                const int command = (filename == "filesystem")
                                        ? filesystemUpdateCommand()
                                        : U_FLASH;
                if (!Update.begin(updateSize(), command)) {
                    Update.printError(Serial);
                    markUploadError(request, "OTA could not begin", 500);
                    return;
                }
                request->setAttribute(kOtaUpdateStarted, true);
                if (!Update.setMD5(expectedMd5.c_str())) {
                    markUploadError(request, "MD5 parameter invalid", 400);
                    return;
                }
            } else {
#if defined(ESP32)
                if (uploadStates.find(request) == uploadStates.end()) {
                    markUploadError(request, "OTA upload state missing", 400);
                    return;
                }
#endif
            }

            if (len) {
#if defined(ESP32) && !defined(EASYNETWORKMANAGER_ALLOW_INSECURE_OTA)
                UploadState& state = uploadStates.at(request);
                if (!state.hashStarted ||
                    mbedtls_sha256_update_ret(&state.sha256, data, len) != 0) {
                    markUploadError(request, "OTA hash update failed", 500);
                    return;
                }
#endif
                if (Update.write(data, len) != len) {
                    markUploadError(request, "OTA could not write", 500);
                    return;
                }
            }

            if (!final) {
                return;
            }

#if defined(ESP32) && !defined(EASYNETWORKMANAGER_ALLOW_INSECURE_OTA)
            UploadState& state = uploadStates.at(request);
            uint8_t digest[32];
            if (mbedtls_sha256_finish_ret(&state.sha256, digest) != 0 ||
                !verifySignature(state.publicKeyPem, state.signature, digest)) {
                markUploadError(request, "OTA signature verification failed",
                                400);
                return;
            }
#endif

            if (!Update.end(true)) {
                Update.printError(Serial);
                markUploadError(request, "Could not end OTA", 500);
                return;
            }
            request->setAttribute(kOtaUpdateStarted, false);
            request->setAttribute(kOtaUploadComplete, true);
            releaseUpload(request);
        }));
}

void AsyncOTA::setOTAHandler(
    AsyncOTACustomHandlerFunction customHandlerFunction) {
    this->customHandlerFunction = customHandlerFunction;
}
