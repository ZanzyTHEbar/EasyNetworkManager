#include "project_config.hpp"

ProjectConfig::ProjectConfig(const std::string &configName, const std::string &mdnsName)
    : _configName(std::move(configName)), _mdnsName(std::move(mdnsName)), _already_loaded(false) {}

ProjectConfig::~ProjectConfig() {}

/**
 *@brief Initializes the structures with blank data to prevent empty memory sectors and nullptr errors.
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
        log_e("MDNS name is null\n Autoassigning name to 'easynetwork'");
        _mdnsName = "easynetwork";
    }
    this->config.mdns = {
        _mdnsName,
        "",
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
    //! No need to save the JSON strings or bools, they are generated and used on the fly
}

void ProjectConfig::mdnsConfigSave() {
    /* MDNS Config */
    putString("hostname", this->config.mdns.hostname.c_str());
    putString("service", this->config.mdns.service.c_str());
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
    ESP.restart();
}

bool ProjectConfig::reset() {
    log_w("Resetting project config");
    return clear();
}

void ProjectConfig::load() {
    log_d("Loading project config");
    if (this->_already_loaded) {
        log_w("Project config already loaded");
        return;
    }

    /* MDNS Config */
    this->config.mdns.hostname.assign(getString("hostname", _mdnsName.c_str()).c_str());
    this->config.mdns.service.assign(getString("service").c_str());
    /* Device Config */
    this->config.device.OTAPassword.assign(getString("OTAPassword", "12345678").c_str());
    this->config.device.OTAPort = getInt("OTAPort", 3232);
    //! No need to load the JSON strings or bools, they are generated and used on the fly

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

        const std::string &temp_1 = getString(name.c_str()).c_str();
        const std::string &temp_2 = getString(ssid.c_str()).c_str();
        const std::string &temp_3 = getString(password.c_str()).c_str();
        uint8_t temp_4 = getUInt(channel.c_str());
        uint8_t temp_5 = getUInt(power.c_str());

        //! push_back creates a copy of the object, so we need to use emplace_back
        this->config.networks.emplace_back(temp_1, temp_2, temp_3, temp_4, temp_5,
                                           false);  // false because the networks we store in the config are the ones we
                                                    // want the esp to connect to, rather than host as AP
    }

    /* AP Config */
    this->config.ap_network.ssid.assign(getString("apSSID", _mdnsName.c_str()).c_str());
    this->config.ap_network.password.assign(getString("apPass", "12345678").c_str());
    this->config.ap_network.channel = getUInt("apChannel", 1);

    this->_already_loaded = true;
    this->notify(ObserverEvent::configLoaded);
}

//**********************************************************************************************************************
//*
//!                                                DeviceConfig
//*
//**********************************************************************************************************************
void ProjectConfig::setDeviceConfig(const std::string &OTAPassword, int OTAPort, bool shouldNotify) {
    log_d("Updating device config");
    this->config.device.OTAPassword.assign(OTAPassword);
    this->config.device.OTAPort = OTAPort;

    if (shouldNotify) {
        this->notify(ObserverEvent::deviceConfigUpdated);
    }
}

void ProjectConfig::setMDNSConfig(const std::string &hostname, const std::string &service, bool shouldNotify) {
    log_d("Updating MDNS config");
    this->config.mdns.hostname.assign(hostname);
    this->config.mdns.service.assign(service);

    if (shouldNotify) this->notify(ObserverEvent::mdnsConfigUpdated);
}

void ProjectConfig::setWifiConfig(const std::string &networkName, const std::string &ssid, const std::string &password,
                                  uint8_t channel, uint8_t power, bool adhoc, bool shouldNotify) {
    // we store the ADHOC flag as false because the networks we store in the config are the ones we want the esp to
    // connect to, rather than host as AP, and here we're just updating them
    size_t size = this->config.networks.size();

    // we're allowing to store up to three additional networks
    if (size == 0) {
        Serial.println("No networks, We're adding a new network");
        this->config.networks.emplace_back(networkName, ssid, password, channel, power, false);
    }

    int networkToUpdate = -1;
    for (int i = 0; i < size; i++) {
        if (strcmp(this->config.networks[i].name.c_str(), networkName.c_str()) == 0) {
            // we've found a preexisting network, let's upate it
            networkToUpdate = i;
            break;
        }
    }

    if (networkToUpdate >= 0) {
        this->config.networks[networkToUpdate].name = networkName;
        this->config.networks[networkToUpdate].ssid = ssid;
        this->config.networks[networkToUpdate].password = password;
        this->config.networks[networkToUpdate].channel = channel;
        this->config.networks[networkToUpdate].power = power;
    } else if (size < 3) {
        Serial.println("We're adding a new network");
        // we don't have that network yet, we can add it as we still have some space
        // we're using emplace_back as push_back will create a copy of it, we want to avoid that
        this->config.networks.emplace_back(networkName, ssid, password, channel, power, false);
    }

    if (shouldNotify) this->notify(ObserverEvent::networksConfigUpdated);
}

void ProjectConfig::setAPWifiConfig(const std::string &ssid, const std::string &password, uint8_t channel, bool adhoc,
                                    bool shouldNotify) {
    this->config.ap_network.ssid.assign(ssid);
    this->config.ap_network.password.assign(password);
    this->config.ap_network.channel = channel;
    this->config.ap_network.adhoc = adhoc;

    log_d("Updating access point config");
    if (shouldNotify) {
        this->notify(ObserverEvent::apConfigUpdated);
    }
}

void ProjectConfig::setWiFiTxPower(uint8_t power, bool shouldNotify) {
    this->config.wifi_tx_power.power = power;

    log_d("Updating wifi tx power");
    if (shouldNotify) this->notify(ObserverEvent::wifiTxPowerUpdated);
}

void ProjectConfig::setDeviceDataJson(const std::string &data, bool shouldNotify) {
    this->config.device_data.deviceJson.assign(data);

    log_d("Updating device data json");
    if (shouldNotify) this->notify(ObserverEvent::deviceDataJsonUpdated);
}

std::string Project_Config::DeviceConfig_t::toRepresentation() {
    std::string json = Helpers::format_string("\"device_config\": {\"OTAPassword\": \"%s\", \"OTAPort\": %u}",
                                              this->OTAPassword.c_str(), this->OTAPort);
    return json;
}

std::string Project_Config::MDNSConfig_t::toRepresentation() {
    std::string json = Helpers::format_string("\"mdns_config\": {\"hostname\": \"%s\", \"service\": \"%s\"}",
                                              this->hostname.c_str(), this->service.c_str());
    return json;
}

std::string Project_Config::WiFiConfig_t::toRepresentation() {
    std::string json = Helpers::format_string(
        "{\"name\": \"%s\", \"ssid\": \"%s\", \"password\": \"%s\", \"channel\": %u, \"adhoc\": %s}",
        this->name.c_str(), this->ssid.c_str(), this->password.c_str(), this->channel, this->power,
        this->adhoc ? "true" : "false");
    return json;
}

std::string Project_Config::AP_WiFiConfig_t::toRepresentation() {
    std::string json = Helpers::format_string(
        "\"ap_wifi_config\": {\"ssid\": \"%s\", \"password\": \"%s\", \"channel\": %u, \"adhoc\": %s}",
        this->ssid.c_str(), this->password.c_str(), this->channel, this->adhoc ? "true" : "false");
    return json;
}

std::string Project_Config::WiFiTxPower_t::toRepresentation() {
    std::string json = Helpers::format_string("\"wifi_tx_power\": {\"power\": %u}", this->power);
    return json;
}

std::string Project_Config::DeviceDataJson_t::toRepresentation() {
    std::string json = Helpers::format_string("\"deviceData\": {\"data\": \"%s\"}", this->deviceJson.c_str());
    return json;
}

//**********************************************************************************************************************
//*
//!                                                GetMethods
//*
//**********************************************************************************************************************

Project_Config::DeviceConfig_t *ProjectConfig::getDeviceConfig() { return &this->config.device; }
Project_Config::MDNSConfig_t *ProjectConfig::getMDNSConfig() { return &this->config.mdns; }
std::vector<Project_Config::WiFiConfig_t> *ProjectConfig::getWifiConfigs() { return &this->config.networks; }
Project_Config::AP_WiFiConfig_t *ProjectConfig::getAPWifiConfig() { return &this->config.ap_network; }
Project_Config::WiFiTxPower_t *ProjectConfig::getWifiTxPowerConfig() { return &this->config.wifi_tx_power; }
Project_Config::DeviceDataJson_t *ProjectConfig::getDeviceDataJson() { return &this->config.device_data; }