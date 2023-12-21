#include <network/wifihandler/wifi_handler.hpp>

WiFiHandler::WiFiHandler(ProjectConfig& configManager, const std::string& ssid,
                         const std::string& password, uint8_t channel)
    : configManager(configManager),
      txpower(this->configManager.getWifiTxPowerConfig()),
      ssid(ssid),
      password(password),
      channel(channel),
      power(0),
      _enable_adhoc(false) {}

WiFiHandler::~WiFiHandler() {}

void WiFiHandler::begin() {
    if (this->_enable_adhoc) {
        this->configManager.setState(this->getName(),
                                     WiFiState_e::WiFiState_ADHOC);
        return;
    }
    WiFi.mode(WIFI_STA);
    WiFi.onEvent(
        std::bind(&WiFiHandler::onWiFiEvent, this, std::placeholders::_1));
    WiFi.setSleep(WIFI_PS_NONE);

    Serial.print("Initializing connection to wifi \n\r");

    auto networks = this->configManager.getWifiConfigs();

    if (networks.empty()) {
        Serial.print(
            "No networks found in config, trying the "
            "default one \n\r");
        if (this->iniSTA(this->ssid, this->password, this->channel,
                         (wifi_power_t)txpower.power)) {
            return;
        }
        Serial.print(
            "Could not connect to the hardcoded "
            "network, setting up ADHOC network \n\r");
        this->configManager.setState(this->getName(),
                                     WiFiState_e::WiFiState_ADHOC);
        return;
    }

    for (auto networkIterator = networks.begin();
         networkIterator != networks.end(); ++networkIterator) {
        if (this->iniSTA(networkIterator->ssid, networkIterator->password,
                         networkIterator->channel,
                         (wifi_power_t)networkIterator->power)) {
            return;
        }
    }

    // at this point, we've tried every network, let's just
    // setup adhoc
    Serial.printf(
        "We've gone through every network, each timed out. "
        "Trying to connect to the hardcoded network one last time: %s \n\r",
        this->ssid.c_str());
    if (this->iniSTA(this->ssid, this->password, this->channel,
                     (wifi_power_t)txpower.power)) {
        Serial.print(
            "Successfully connected to the hardcoded "
            "network. \n\r");
        return;
    }
    log_e(
        "Could not connect to the hardcoded network, "
        "setting up adhoc. \n\r");
    this->configManager.setState(this->getName(), WiFiState_e::WiFiState_ADHOC);
}

void WiFiHandler::adhoc(const std::string& ssid, uint8_t channel,
                        const std::string& password) {
    log_i("\n[INFO]: Setting Access Point...\n");
    log_i("\n[INFO]: Configuring access point...\n");
    WiFi.mode(WIFI_AP);
    WiFi.setSleep(WIFI_PS_NONE);
    Serial.printf("\r\nStarting AP. \r\nAP IP address: ");
    IPAddress IP = WiFi.softAPIP();
    Serial.printf("[INFO]: AP IP address: %s.\r\n", IP.toString().c_str());
    // You can remove the password parameter if you want the
    // AP to be open.
    WiFi.softAP(ssid.c_str(), password.c_str(),
                channel);  // AP mode with password
    WiFi.setTxPower((wifi_power_t)txpower.power);
}

void WiFiHandler::setUpADHOC() {
    log_i("\n[INFO]: Setting Access Point...\n");
    size_t ssidLen = this->configManager.getAPWifiConfig().ssid.length();
    size_t passwordLen =
        this->configManager.getAPWifiConfig().password.length();
    if (ssidLen <= 0) {
        this->adhoc("EasyNetworkManager", 1, "12345678");
        return;
    }

    if (passwordLen <= 0) {
        log_i(
            "\n[INFO]: Configuring access point without a "
            "password\n");
        this->adhoc(this->configManager.getAPWifiConfig().ssid,
                    this->configManager.getAPWifiConfig().channel);
        return;
    }

    this->adhoc(this->configManager.getAPWifiConfig().ssid,
                this->configManager.getAPWifiConfig().channel,
                this->configManager.getAPWifiConfig().password);
    log_i("\n[INFO]: Configuring access point...\n");
    log_d("\n[DEBUG]: ssid: %s\n",
          this->configManager.getAPWifiConfig().ssid.c_str());
    log_d("\n[DEBUG]: password: %s\n",
          this->configManager.getAPWifiConfig().password.c_str());
    log_d("\n[DEBUG]: channel: %d\n",
          this->configManager.getAPWifiConfig().channel);
}

bool WiFiHandler::iniSTA(const std::string& ssid, const std::string& password,
                         uint8_t channel, wifi_power_t power) {
    unsigned long currentMillis = millis();
    unsigned long startingMillis = currentMillis;
    int connectionTimeout = 45000;  // 30 seconds
    int progress = 0;
    Serial.printf("Trying to connect to: %s \n\r", ssid.c_str());
    auto mdnsConfig = this->configManager.getMDNSConfig();
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE,
                INADDR_NONE);  // need to call before
                               // setting hostname
    WiFi.setHostname(mdnsConfig.hostname.c_str());
    WiFi.begin(ssid.c_str(), password.c_str(), channel);
    while (WiFi.status() != WL_CONNECTED) {
        progress++;
        currentMillis = millis();
        // Helpers::update_progress_bar(progress, 100);
        delay(1000);
        log_v(".");
        if ((currentMillis - startingMillis) >= connectionTimeout) {
            this->configManager.setState(this->getName(),
                                         WiFiState_e::WiFiState_Error);
            log_e("Connection to: %s TIMEOUT \n\r", ssid.c_str());
            delay(8000);
            return false;
        }
    }

    this->configManager.setState(this->getName(),
                                 WiFiState_e::WiFiState_Connected);
    Serial.printf("Successfully connected to %s \n\r", ssid.c_str());
    // Serial.printf("Setting TX power to: %d \n\r", (uint8_t)power);
    // WiFi.setTxPower(power);

    return true;
}

void WiFiHandler::toggleAdhoc(bool enable) {
    _enable_adhoc = enable;
}

void WiFiHandler::update(const StateVariant& event) {
    updateWrapper<WiFiState_e>(event, [this](WiFiState_e _event) {
        switch (_event) {
            case WiFiState_e::WiFiState_Connecting:
                Serial.print("WiFiState_Connecting \n\r");
                this->begin();
                break;
            case WiFiState_e::WiFiState_Connected:
                Serial.print("WiFiState_Connected \n\r");
                break;
            case WiFiState_e::WiFiState_Disconnected:
                Serial.print("WiFiState_Disconnected \n\r");
                break;
            case WiFiState_e::WiFiState_Error:
                Serial.print("WiFiState_Error \n\r");
                break;
            case WiFiState_e::WiFiState_ADHOC:
                Serial.print("WiFiState_ADHOC \n\r");
                this->setUpADHOC();
                break;
            default:
                break;
        }
    });

    // updateWrapper<Event_e>(event, [this](Event_e _event) {
    //     switch (_event) {
    //         case Event_e::configSaved:
    //             this->begin();
    //             break;
    //         default:
    //             break;
    //     }
    // });
}
void WiFiHandler::onWiFiEvent(WiFiEvent_t event) {
    switch (event) {
        /* case SYSTEM_EVENT_WIFI_READY:
            this->configManager.setState(this->getName(),
                                         WiFiState_e::WiFiState_Idle);
            break;
        case SYSTEM_EVENT_SCAN_DONE:
            this->configManager.setState(this->getName(),
                                         WiFiState_e::WiFiState_Scanning_Done);
            break; */
        case SYSTEM_EVENT_STA_DISCONNECTED:
            this->configManager.setState(this->getName(),
                                         WiFiState_e::WiFiState_Disconnected);
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            this->configManager.setState(this->getName(),
                                         WiFiState_e::WiFiState_Connected);
            break;
    }
}
