#include <network/wifihandler/wifihandler.hpp>

WiFiHandler::WiFiHandler(ProjectConfig& configManager, const std::string& ssid,
                         const std::string& password, uint8_t channel)
    : configManager(configManager),
      txpower(configManager.getWifiTxPowerConfig()),
      ssid(ssid),
      password(password),
      channel(channel),
      power(0),
      _enable_adhoc(false) {}

WiFiHandler::~WiFiHandler() {}

void WiFiHandler::begin() {
    if (this->_enable_adhoc ||
        wifiStateManager.getCurrentState() == WiFiState_e::WiFiState_ADHOC) {
        this->setUpADHOC();
        return;
    }
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(WIFI_PS_NONE);

    Serial.print("Initializing connection to wifi \n\r");
    wifiStateManager.setState(WiFiState_e::WiFiState_Connecting);

    auto networks = configManager.getWifiConfigs();

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
        this->setUpADHOC();
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
    this->setUpADHOC();
}

void WiFiHandler::adhoc(const std::string& ssid, uint8_t channel,
                        const std::string& password) {
    wifiStateManager.setState(WiFiState_e::WiFiState_ADHOC);
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
    size_t ssidLen = configManager.getAPWifiConfig().ssid.length();
    size_t passwordLen = configManager.getAPWifiConfig().password.length();
    if (ssidLen <= 0) {
        this->adhoc("EasyNetworkManager", 1, "12345678");
        return;
    }

    if (passwordLen <= 0) {
        log_i(
            "\n[INFO]: Configuring access point without a "
            "password\n");
        this->adhoc(configManager.getAPWifiConfig().ssid,
                    configManager.getAPWifiConfig().channel);
        return;
    }

    this->adhoc(configManager.getAPWifiConfig().ssid,
                configManager.getAPWifiConfig().channel,
                configManager.getAPWifiConfig().password);
    log_i("\n[INFO]: Configuring access point...\n");
    log_d("\n[DEBUG]: ssid: %s\n",
          configManager.getAPWifiConfig().ssid.c_str());
    log_d("\n[DEBUG]: password: %s\n",
          configManager.getAPWifiConfig().password.c_str());
    log_d("\n[DEBUG]: channel: %d\n", configManager.getAPWifiConfig().channel);
}

bool WiFiHandler::iniSTA(const std::string& ssid, const std::string& password,
                         uint8_t channel, wifi_power_t power) {
    unsigned long currentMillis = millis();
    unsigned long startingMillis = currentMillis;
    int connectionTimeout = 30000;  // 30 seconds
    int progress = 0;

    wifiStateManager.setState(WiFiState_e::WiFiState_Connecting);
    Serial.printf("Trying to connect to: %s \n\r", ssid.c_str());
    auto mdnsConfig = configManager.getMDNSConfig();
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE,
                INADDR_NONE);  // need to call before
                               // setting hostname
    WiFi.setHostname(mdnsConfig.hostname.c_str());
    WiFi.begin(ssid.c_str(), password.c_str(), channel);
    while (WiFi.status() != WL_CONNECTED) {
        progress++;
        currentMillis = millis();
        Helpers::update_progress_bar(progress, 100);
        delay(301);
        if ((currentMillis - startingMillis) >= connectionTimeout) {
            wifiStateManager.setState(WiFiState_e::WiFiState_Error);
            log_e("Connection to: %s TIMEOUT \n\r", ssid.c_str());
            return false;
        }
    }

    wifiStateManager.setState(WiFiState_e::WiFiState_Connected);
    Serial.printf("Successfully connected to %s \n\r", ssid.c_str());
    Serial.printf("Setting TX power to: %d \n\r", (uint8_t)power);
    WiFi.setTxPower(power);

    return true;
}

void WiFiHandler::toggleAdhoc(bool* enable) {
    _enable_adhoc = enable;
}

void WiFiHandler::update(Event_e event) {
    switch (event) {
        case Event_e::networksConfigUpdated:
            this->begin();
            break;
        default:
            break;
    }
}

std::string WiFiHandler::getName() {
    return "WiFiHandler";
}