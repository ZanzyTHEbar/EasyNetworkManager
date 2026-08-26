#include <network/wifihandler/wifi_handler.hpp>

WiFiHandler::WiFiHandler(ProjectConfig& configManager, const std::string& ssid,
                         const std::string& password, uint8_t channel)
    : configManager(configManager),
      txpower(this->configManager.getWifiTxPowerConfig()),
      ssid(ssid),
      password(password),
      channel(channel),
      power(0),
      _enable_adhoc(false),
      _connectState(AsyncConnectState::Idle),
      _attemptStartMs(0),
      _candidateIndex(0),
      _fallbackAttempted(false),
#if defined(ARDUINO_ARCH_ESP32)
      _wifiEventId(0),
#elif defined(ARDUINO_ARCH_ESP8266)
      _gotIpHandler(),
      _disconnectedHandler(),
#endif
      _wifiEventRegistered(false) {
    this->setID(static_cast<uint64_t>(
        ProjectConfigEventIDs_e::ProjectConfigEventID_WifiHandler));
    this->setLabel("WiFiHandler");
}

WiFiHandler::~WiFiHandler() {
#if defined(ARDUINO_ARCH_ESP32)
    if (_wifiEventRegistered) {
        WiFi.removeEvent(_wifiEventId);
        _wifiEventRegistered = false;
    }
#elif defined(ARDUINO_ARCH_ESP8266)
    _gotIpHandler.reset();
    _disconnectedHandler.reset();
    _wifiEventRegistered = false;
#endif
}

void WiFiHandler::begin() {
    if (this->_enable_adhoc) {
        this->configManager.notifyAll(WiFiState_e::WiFiState_ADHOC);
        return;
    }
    WiFi.mode(WIFI_STA);
#if defined(ARDUINO_ARCH_ESP32)
    if (!_wifiEventRegistered) {
        _wifiEventId = WiFi.onEvent(
            std::bind(&WiFiHandler::onWiFiEvent, this, std::placeholders::_1));
        _wifiEventRegistered = true;
    }
#elif defined(ARDUINO_ARCH_ESP8266)
    if (!_wifiEventRegistered) {
        _gotIpHandler = WiFi.onStationModeGotIP(
            [this](const WiFiEventStationModeGotIP& event) {
                this->onStationModeGotIP(event);
            });
        _disconnectedHandler = WiFi.onStationModeDisconnected(
            [this](const WiFiEventStationModeDisconnected& event) {
                this->onStationModeDisconnected(event);
            });
        _wifiEventRegistered = true;
    }
#endif
    WiFi.setSleep(WIFI_PS_NONE);

    this->log("Initializing connection to wifi networks...");

    /* Kick off the first candidate and return immediately; loop()
     * advances the state machine from here on. */
    this->_candidateIndex = 0;
    this->_fallbackAttempted = false;
    this->startNextAttempt();
}

void WiFiHandler::loop() {
    if (this->_connectState != AsyncConnectState::Connecting) {
        return;
    }

    if (WiFi.status() == WL_CONNECTED) {
        this->_connectState = AsyncConnectState::Connected;
        /* The Connected event itself is emitted by the Wi-Fi
         * event listener. */
        this->log("Successfully connected to the configured network.");
        return;
    }

    if ((millis() - this->_attemptStartMs) >=
        EASYNETWORKMANAGER_WIFI_CONNECT_TIMEOUT_MS) {
        log_e("Connection attempt TIMEOUT after %lu ms",
              (unsigned long)EASYNETWORKMANAGER_WIFI_CONNECT_TIMEOUT_MS);
        this->configManager.notifyAll(WiFiState_e::WiFiState_Error);
        this->startNextAttempt();
    }
}

/* Advance to the next candidate network: stored configs first, then
 * the constructor fallback credentials once, then the AP fallback. */
void WiFiHandler::startNextAttempt() {
    auto& networks = this->configManager.getWifiConfigs();

    while (this->_candidateIndex < networks.size()) {
        const auto& network = networks[this->_candidateIndex++];
        if (!network.ssid.empty()) {
            this->startAttempt(network.ssid, network.password,
                               network.channel);
            return;
        }
    }

    if (!this->_fallbackAttempted && !this->ssid.empty()) {
        this->_fallbackAttempted = true;
        this->log(
            "No stored network connected, trying the "
            "default one ...");
        this->startAttempt(this->ssid, this->password, this->channel);
        return;
    }

    // at this point, we've tried every candidate, let's just setup adhoc
    this->log(
        "We've gone through every network; setting up "
        "the ADHOC fallback ...");
    this->_connectState = AsyncConnectState::ApFallback;
    this->log("WiFiState_ADHOC");
    /* Routed through update() -> setUpADHOC(), which validates the AP
     * credentials before bringing the access point up. */
    this->configManager.notifyAll(WiFiState_e::WiFiState_ADHOC);
}

void WiFiHandler::startAttempt(const std::string& ssid,
                               const std::string& password,
                               uint8_t channel) {
    this->log("Trying to connect to: ", ssid);
    auto mdnsConfig = this->configManager.getMDNSConfig();
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(mdnsConfig.hostname.c_str());
    WiFi.begin(ssid.c_str(), password.c_str(), channel);
    this->_attemptStartMs = millis();
    this->_connectState = AsyncConnectState::Connecting;
    this->configManager.notifyAll(WiFiState_e::WiFiState_Connecting);
}

void WiFiHandler::adhoc(const std::string& ssid, uint8_t channel,
                        const std::string& password) {
    this->log("Configuring access point...\n");
    WiFi.mode(WIFI_AP);
    WiFi.setSleep(WIFI_PS_NONE);
    this->log("Starting AP...");
    IPAddress IP = WiFi.softAPIP();
    this->log("AP IP address: ", IP.toString().c_str());
    // You can remove the password parameter if you want the
    // AP to be open.
    WiFi.softAP(ssid.c_str(), password.c_str(),
                channel);  // AP mode with password
#if defined(ARDUINO_ARCH_ESP32)
    WiFi.setTxPower((wifi_power_t)txpower.power);
#elif defined(ARDUINO_ARCH_ESP8266)
    WiFi.setOutputPower(txpower.power / 4.0f);
#endif
}

void WiFiHandler::setUpADHOC() {
    this->log("Setting Access Point... ");
    size_t ssidLen = this->configManager.getAPWifiConfig().ssid.length();
    size_t passwordLen =
        this->configManager.getAPWifiConfig().password.length();
    if (ssidLen <= 0 || passwordLen < 8) {
        log_e("Access point SSID and a password of at least 8 characters are "
              "required; AP startup aborted");
        return;
    }

    this->adhoc(this->configManager.getAPWifiConfig().ssid,
                this->configManager.getAPWifiConfig().channel,
                this->configManager.getAPWifiConfig().password);
    this->log("Configuring access point...");
    log_d("\n[DEBUG]: ssid: %s\n",
          this->configManager.getAPWifiConfig().ssid.c_str());
    log_d("\n[DEBUG]: channel: %d\n",
          this->configManager.getAPWifiConfig().channel);
}

void WiFiHandler::toggleAdhoc(bool enable) {
    _enable_adhoc = enable;
}

void WiFiHandler::update(const StateVariant& event) {
    updateStateWrapper<WiFiState_e>(event, [this](WiFiState_e _event) {
        switch (_event) {
            case WiFiState_e::WiFiState_Connecting:
                /* Emitted by startAttempt(); the connect itself is
                 * driven from loop(), nothing to do here. */
                this->log("WiFiState_Connecting");
                break;
            case WiFiState_e::WiFiState_Connected:
                this->log("WiFiState_Connected");
                break;
            case WiFiState_e::WiFiState_Disconnected:
                this->log("WiFiState_Disconnected");
                break;
            case WiFiState_e::WiFiState_Error:
                this->log("WiFiState_Error");
                break;
            case WiFiState_e::WiFiState_ADHOC:
                this->log("WiFiState_ADHOC");
                this->setUpADHOC();
                break;
            default:
                break;
        }

        // FIXME: This creates a stack overflow for some reason
        // if (this->customHandlerFunction) {
        //    this->customHandlerFunction(_event);
        //}
    });

    // updateStateWrapper<Event_e>(event, [this](Event_e _event) {
    //     switch (_event) {
    //         case Event_e::configSaved:
    //             this->begin();
    //             break;
    //         default:
    //             break;
    //     }
    // });
}
#if defined(ARDUINO_ARCH_ESP32)
void WiFiHandler::onWiFiEvent(WiFiEvent_t event) {
    switch (event) {
        /* case SYSTEM_EVENT_WIFI_READY:
            this->configManager.notifyAll(
                                         WiFiState_e::WiFiState_Idle);
            break;
        case SYSTEM_EVENT_SCAN_DONE:
            this->configManager.notifyAll(
                                         WiFiState_e::WiFiState_Scanning_Done);
            break; */
        case SYSTEM_EVENT_STA_DISCONNECTED:
            this->configManager.notifyAll(WiFiState_e::WiFiState_Disconnected);
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            this->configManager.notifyAll(WiFiState_e::WiFiState_Connected);
            break;
    }
}
#elif defined(ARDUINO_ARCH_ESP8266)
void WiFiHandler::onStationModeGotIP(
    const WiFiEventStationModeGotIP& event) {
    (void)event;
    this->configManager.notifyAll(WiFiState_e::WiFiState_Connected);
}

void WiFiHandler::onStationModeDisconnected(
    const WiFiEventStationModeDisconnected& event) {
    (void)event;
    this->configManager.notifyAll(WiFiState_e::WiFiState_Disconnected);
}
#endif

void WiFiHandler::setCustomHandler(
    WiFiHandlerCustomHandlerFunction customHandlerFunction) {
    this->customHandlerFunction = customHandlerFunction;
}
