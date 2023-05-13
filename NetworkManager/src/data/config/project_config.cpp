#include <data/config/project_config.hpp>

ProjectConfig::ProjectConfig(const std::string& configName,
                             const std::string& mdnsName)
    : _configName(std::move(configName)),
      _mdnsName(std::move(mdnsName)),
      _already_loaded(false) {}

ProjectConfig::~ProjectConfig() {}

/**
 *@brief Initializes the structures with blank data to prevent empty memory
 *sectors and nullptr errors.
 *@brief This is to be called in setup() before loading the config.
 */
void ProjectConfig::initConfig() {
    if (_configName.empty()) {
        log_e("Config name is null\n");
        _configName = "easynetwork";
    }

    bool successs = begin(_configName.c_str());

    log_i("Config name: %s", _configName.c_str());
    log_i("Config loaded: %s", successs ? "true" : "false");

    this->config.device = {
        "12345678",
        3232,
    };

    if (_mdnsName.empty()) {
        log_e("MDNS name is null\n Auto-assigning name to 'easynetwork'");
        _mdnsName = "easynetwork";
    }
    this->config.mdns = {
        _mdnsName,
    };

    log_i("MDNS name: %s", _mdnsName.c_str());

    this->config.ap_network = {
        "",
        "",
        1,
        false,
    };
}

void ProjectConfig::deviceConfigSave() {
    /* Device Config */
    putString("OTAPassword", this->config.device.OTAPassword.c_str());
    putInt("OTAPort", this->config.device.OTAPort);
    //! No need to save the JSON strings or bools, they are generated and used
    //! on the fly
}

void ProjectConfig::mdnsConfigSave() {
    /* MDNS Config */
    putString("hostname", this->config.mdns.hostname.c_str());
}

void ProjectConfig::wifiConfigSave() {
    /* WiFi Config */
    putInt("networkCount", this->config.networks.size());

    std::string name = "name";
    std::string ssid = "ssid";
    std::string password = "pass";
    std::string channel = "channel";
    std::string power = "power";
    for (int i = 0; i < this->config.networks.size(); i++) {
        char buffer[2];
        std::string iter_str = Helpers::itoa(i, buffer, 10);

        name.append(iter_str);
        ssid.append(iter_str);
        password.append(iter_str);
        channel.append(iter_str);
        power.append(iter_str);

        putString(name.c_str(), this->config.networks[i].name.c_str());
        putString(ssid.c_str(), this->config.networks[i].ssid.c_str());
        putString(password.c_str(), this->config.networks[i].password.c_str());
        putUInt(channel.c_str(), this->config.networks[i].channel);
        putUInt(power.c_str(), this->config.networks[i].power);
    }

    /* AP Config */
    putString("apSSID", this->config.ap_network.ssid.c_str());
    putString("apPass", this->config.ap_network.password.c_str());
    putUInt("apChannel", this->config.ap_network.channel);
}

void ProjectConfig::wifiTxPowerConfigSave() {
    /* Device Config */
    putInt("power", this->config.wifi_tx_power.power);
}

void ProjectConfig::save() {
    log_d("Saving project config");
    deviceConfigSave();
    wifiConfigSave();
    wifiTxPowerConfigSave();
    mdnsConfigSave();
    end();
    log_i("Project config saved and system is rebooting");
    int initialTime = millis();
    while (millis() - initialTime <= 2000) {
        continue;
    }
    ESP.restart();
}

bool ProjectConfig::reset() {
    log_w("Resetting project config");
    return clear();
}

void ProjectConfig::load() {
    log_d("Loading project config");
    // call before load to initialise the structs
    initConfig();
    if (this->_already_loaded) {
        log_w("Project config already loaded");
        return;
    }

    /* MDNS Config */
    this->config.mdns.hostname.assign(
        getString("hostname", _mdnsName.c_str()).c_str());
    /* Device Config */
    this->config.device.OTAPassword.assign(
        getString("OTAPassword", "12345678").c_str());
    this->config.device.OTAPort = getInt("OTAPort", 3232);
    //! No need to load the JSON strings or bools, they are generated and used
    //! on the fly

    /* Wifi TX Power Config */
    this->config.wifi_tx_power.power = getUInt("power", 52);
    /* WiFi Config */
    int networkCount = getInt("networkCount", 0);
    std::string name = "name";
    std::string ssid = "ssid";
    std::string password = "pass";
    std::string channel = "channel";
    std::string power = "power";
    for (int i = 0; i < networkCount; i++) {
        char buffer[2];
        std::string iter_str = Helpers::itoa(i, buffer, 10);

        name.append(iter_str);
        ssid.append(iter_str);
        password.append(iter_str);
        channel.append(iter_str);
        power.append(iter_str);

        const std::string& temp_1 = getString(name.c_str()).c_str();
        const std::string& temp_2 = getString(ssid.c_str()).c_str();
        const std::string& temp_3 = getString(password.c_str()).c_str();
        uint8_t temp_4 = getUInt(channel.c_str());
        uint8_t temp_5 = getUInt(power.c_str());

        //! push_back creates a copy of the object, so we need to use
        //! emplace_back
        this->config.networks.emplace_back(
            temp_1, temp_2, temp_3, temp_4, temp_5,
            false);  // false because the networks we store in the config are
                     // the ones we want the esp to connect to, rather than host
                     // as AP
    }

    /* AP Config */
    this->config.ap_network.ssid.assign(
        getString("apSSID", _mdnsName.c_str()).c_str());
    this->config.ap_network.password.assign(
        getString("apPass", "12345678").c_str());
    this->config.ap_network.channel = getUInt("apChannel", 1);

    this->_already_loaded = true;
    this->notifyAll(Event_e::configLoaded);
}

//**********************************************************************************************************************
//*
//!                                                DeviceConfig
//*
//**********************************************************************************************************************
void ProjectConfig::setDeviceConfig(const std::string& OTAPassword, int OTAPort,
                                    bool shouldNotify) {
    log_d("Updating device config");
    this->config.device.OTAPassword.assign(OTAPassword);
    this->config.device.OTAPort = OTAPort;

    if (shouldNotify) {
        this->notifyAll(Event_e::deviceConfigUpdated);
    }
}

bool ProjectConfig::setMDNSConfig(const std::string& hostname,
                                  bool shouldNotify) {
    log_d("Updating MDNS config");
    for (int i = 0; i < this->config.mdns.hostname.size(); i++) {
        if (this->config.mdns.hostname[i] == '-' ||
            this->config.mdns.hostname[i] == '.')
            continue;
        else if (this->config.mdns.hostname[i] >= '0' &&
                 this->config.mdns.hostname[i] <= '9')
            continue;
        else if (this->config.mdns.hostname[i] >= 'A' &&
                 this->config.mdns.hostname[i] <= 'Z')
            continue;
        else if (this->config.mdns.hostname[i] >= 'a' &&
                 this->config.mdns.hostname[i] <= 'z')
            continue;
        else if (this->config.mdns.hostname[i] == 0 && i > 0)
            break;
        log_i("Invalid hostname, please use only alphanumeric characters");
        return false;
    }
    this->config.mdns.hostname.assign(hostname);

    if (shouldNotify)
        this->notifyAll(Event_e::mdnsConfigUpdated);
    return true;
}

void ProjectConfig::setWifiConfig(const std::string& networkName,
                                  const std::string& ssid,
                                  const std::string& password, uint8_t channel,
                                  uint8_t power, bool adhoc,
                                  bool shouldNotify) {
    // we store the ADHOC flag as false because the networks we store in the
    // config are the ones we want the esp to connect to, rather than host as
    // AP, and here we're just updating them
    size_t size = this->config.networks.size();
    // we're allowing to store up to three additional networks
    if (size == 0) {
        Serial.println("No networks, We're adding a new network");
        this->config.networks.emplace_back(networkName, ssid, password, channel,
                                           power, false);

        if (shouldNotify)
            this->notifyAll(Event_e::networksConfigUpdated);

        return;
    }

    for (auto it = this->config.networks.begin();
         it != this->config.networks.end();) {
        if (it->name == networkName) {
            log_i("Found network %s, updating it ...", it->name.c_str());

            it->name = networkName;
            it->ssid = ssid;
            it->password = password;
            it->channel = channel;
            it->power = power;
            it->adhoc = false;

            if (shouldNotify)
                this->notifyAll(Event_e::networksConfigUpdated);

            return;
        } else {
            ++it;
        }
    }

    if (size < 3) {
        Serial.println("We're adding a new network");
        // we don't have that network yet, we can add it as we still have some
        // space we're using emplace_back as push_back will create a copy of it,
        // we want to avoid that
        this->config.networks.emplace_back(networkName, ssid, password, channel,
                                           power, false);
    }

    if (shouldNotify)
        this->notifyAll(Event_e::networksConfigUpdated);
}

void ProjectConfig::deleteWifiConfig(const std::string& networkName,
                                     bool shouldNotify) {
    size_t size = this->config.networks.size();
    if (size == 0) {
        Serial.println("No networks, nothing to delete");
    }

    for (auto it = this->config.networks.begin();
         it != this->config.networks.end();) {
        if (it->name == networkName) {
            log_i("Found network %s", it->name.c_str());
            it = this->config.networks.erase(it);
            log_i("Deleted network %s", networkName.c_str());

        } else {
            ++it;
        }
    }

    if (shouldNotify)
        this->notifyAll(Event_e::networksConfigUpdated);
}

void ProjectConfig::setAPWifiConfig(const std::string& ssid,
                                    const std::string& password,
                                    uint8_t channel, bool adhoc,
                                    bool shouldNotify) {
    this->config.ap_network.ssid.assign(ssid);
    this->config.ap_network.password.assign(password);
    this->config.ap_network.channel = channel;
    this->config.ap_network.adhoc = adhoc;

    log_d("Updating access point config");
    if (shouldNotify) {
        this->notifyAll(Event_e::apConfigUpdated);
    }
}

void ProjectConfig::setWiFiTxPower(uint8_t power, bool shouldNotify) {
    this->config.wifi_tx_power.power = power;

    log_d("Updating wifi tx power");
    if (shouldNotify)
        this->notifyAll(Event_e::wifiTxPowerUpdated);
}

void ProjectConfig::setDeviceDataJson(const std::string& data,
                                      bool shouldNotify) {
    this->config.device_data.deviceJson.assign(data);

    log_d("Updating device data json");
    if (shouldNotify)
        this->notifyAll(Event_e::deviceDataJsonUpdated);
}

std::string Project_Config::DeviceConfig_t::toRepresentation() {
    std::string json = Helpers::format_string(
        "\"device_config\": {\"OTAPassword\": \"%s\", \"OTAPort\": %u}",
        this->OTAPassword.c_str(), this->OTAPort);
    return json;
}

std::string Project_Config::MDNSConfig_t::toRepresentation() {
    std::string json = Helpers::format_string(
        "\"mdns_config\": {\"hostname\": \"%s\"}", this->hostname.c_str());
    return json;
}

std::string Project_Config::WiFiConfig_t::toRepresentation() {
    std::string json = Helpers::format_string(
        "{\"name\": \"%s\", \"ssid\": \"%s\", \"password\": \"%s\", "
        "\"channel\": %u, \"power\": %u, \"adhoc\": %s}",
        this->name.c_str(), this->ssid.c_str(), this->password.c_str(),
        this->channel, this->power, this->adhoc ? "true" : "false");
    return json;
}

std::string Project_Config::AP_WiFiConfig_t::toRepresentation() {
    std::string json = Helpers::format_string(
        "\"ap_wifi_config\": {\"ssid\": \"%s\", \"password\": \"%s\", "
        "\"channel\": %u, \"adhoc\": %s}",
        this->ssid.c_str(), this->password.c_str(), this->channel,
        this->adhoc ? "true" : "false");
    return json;
}

std::string Project_Config::WiFiTxPower_t::toRepresentation() {
    std::string json = Helpers::format_string(
        "\"wifi_tx_power\": {\"power\": %u}", this->power);
    return json;
}

std::string Project_Config::DeviceDataJson_t::toRepresentation() {
    std::string json = Helpers::format_string(
        "\"deviceData\": {\"data\": \"%s\"}", this->deviceJson.c_str());
    return json;
}

//**********************************************************************************************************************
//*
//!                                                GetMethods
//*
//**********************************************************************************************************************

Project_Config::DeviceConfig_t& ProjectConfig::getDeviceConfig() {
    return this->config.device;
}
Project_Config::MDNSConfig_t& ProjectConfig::getMDNSConfig() {
    return this->config.mdns;
}
std::vector<Project_Config::WiFiConfig_t>& ProjectConfig::getWifiConfigs() {
    return this->config.networks;
}
Project_Config::AP_WiFiConfig_t& ProjectConfig::getAPWifiConfig() {
    return this->config.ap_network;
}
Project_Config::WiFiTxPower_t& ProjectConfig::getWifiTxPowerConfig() {
    return this->config.wifi_tx_power;
}
Project_Config::DeviceDataJson_t& ProjectConfig::getDeviceDataJson() {
    return this->config.device_data;
}