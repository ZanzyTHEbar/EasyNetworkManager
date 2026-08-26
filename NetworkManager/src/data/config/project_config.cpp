#include <data/config/project_config.hpp>

#include <algorithm>
#include <utility>

namespace {

constexpr char kSnapshotKey[] = "config_snapshot";
constexpr char kSnapshotAuthorityKey[] = "config_snapshot_authority";
constexpr char kSnapshotAuthorityValue[] = "snapshot-v1";
constexpr int kDefaultOtaPort = 3232;
constexpr uint8_t kDefaultWifiChannel = 1;
constexpr uint8_t kDefaultWifiPower = 52;

}  // namespace

ProjectConfig::ProjectConfig(const std::string& configName,
                             const std::string& mdnsName)
    : _configName(std::move(configName)),
      _mdnsName(std::move(mdnsName)),
      _already_loaded(false),
      _custom_config_interface(nullptr),
      reboot(false) {
    this->setLabel("ProjectConfig");
}

ProjectConfig::~ProjectConfig() {
    this->detachAll();
}

void ProjectConfig::initConfig() {
    if (_configName.empty()) {
        log_e("Config name is null\n");
        _configName = "easynetwork";
    }

    bool success = begin(_configName.c_str());
    preferencesOpen = success;

    if (_mdnsName.empty()) {
        log_e(
            "MDNS name is null\n Auto-assigning name to "
            "'easynetwork'");
        _mdnsName = "easynetwork";
    }

    this->config.mdns.hostname.assign(_mdnsName);

    log_i("MDNS name: %s", _mdnsName.c_str());
    log_i("Config name: %s", _configName.c_str());
    log_i("Config loaded: %s", success ? "true" : "false");
}

void ProjectConfig::deviceConfigSave() {
    // save() closes storage after persisting; reopen it for this operation.
    if (!preferencesOpen) {
        preferencesOpen = begin(_configName.c_str());
    }
    if (!preferencesOpen) {
        recordPersistenceResult(0);
        return;
    }
    recordPersistenceResult(persistSnapshot() ? 1 : 0);
    end();
    preferencesOpen = false;
}

void ProjectConfig::mdnsConfigSave() {
    if (!preferencesOpen) {
        preferencesOpen = begin(_configName.c_str());
    }
    if (!preferencesOpen) {
        recordPersistenceResult(0);
        return;
    }
    recordPersistenceResult(persistSnapshot() ? 1 : 0);
    end();
    preferencesOpen = false;
}

void ProjectConfig::wifiConfigSave() {
    if (!preferencesOpen) {
        preferencesOpen = begin(_configName.c_str());
    }
    if (!preferencesOpen) {
        recordPersistenceResult(0);
        return;
    }
    recordPersistenceResult(persistSnapshot() ? 1 : 0);
    end();
    preferencesOpen = false;
}

void ProjectConfig::wifiTxPowerConfigSave() {
    if (!preferencesOpen) {
        preferencesOpen = begin(_configName.c_str());
    }
    if (!preferencesOpen) {
        recordPersistenceResult(0);
        return;
    }
    recordPersistenceResult(persistSnapshot() ? 1 : 0);
    end();
    preferencesOpen = false;
}

bool ProjectConfig::persistSnapshot() {
    config_snapshot snapshot;
    snapshot.device = this->config.device;
    snapshot.device_data = this->config.device_data;
    snapshot.mdns = this->config.mdns;
    snapshot.wifi_tx_power = this->config.wifi_tx_power;
    snapshot.ap_network = this->config.ap_network;
    snapshot.networks = this->config.networks;

    std::string serializedSnapshot;
    if (!serializeSnapshot(snapshot, serializedSnapshot)) {
        log_e("Project config snapshot serialization failed");
        return false;
    }

    if (putString(kSnapshotKey, serializedSnapshot.c_str()) == 0) {
        return false;
    }

    if (putString(kSnapshotAuthorityKey, kSnapshotAuthorityValue) == 0) {
        return false;
    }

    persistenceAuthority = ConfigPersistenceAuthority::Snapshot;
    return true;
}

void ProjectConfig::save() {
    log_d("Saving project config");

    if (!preferencesOpen) {
        preferencesOpen = begin(_configName.c_str());
        if (!preferencesOpen) {
            persistenceFailed = true;
            end();
            return;
        }
    }

    persistenceFailed = !persistSnapshot();

    if (persistenceFailed) {
        log_e("Project config persistence failed; configSaved not emitted");
        end();
        preferencesOpen = false;
        return;
    }

    /* Custom save */
    if (_custom_config_interface != nullptr)
        _custom_config_interface->save();

    // No settle wait: persistSnapshot() commits synchronously, and this runs
    // on the async_tcp task where delay()/spinning are both unsafe.
    // Close NVS
    end();
    preferencesOpen = false;

    if (this->reboot) {
        log_i("Project config saved and system is rebooting");
        ESP.restart();
        return;
    }

    log_w("Reboot is disabled, triggering observer");
    this->_already_loaded = false;
    this->notifyAll(Event_e::configSaved);
}

void ProjectConfig::recordPersistenceResult(size_t bytesWritten) {
    persistenceFailed = bytesWritten == 0;
}

bool ProjectConfig::reset() {
    log_w("Resetting project config");
    if (!preferencesOpen) {
        preferencesOpen = begin(_configName.c_str());
    }

    bool success = false;
    if (preferencesOpen) {
        success = clear();
        end();
        preferencesOpen = false;
    }
    persistenceFailed = !success;
    if (success) {
        this->config = Project_Config::ProjectConfig_t();
        this->config.mdns.hostname = _mdnsName;
        this->_already_loaded = false;
        persistenceAuthority = ConfigPersistenceAuthority::Legacy;
        // Only tell observers to reload once storage reset succeeded.
        this->notifyAll(Event_e::configSaved);
    }
    return success;
}

void ProjectConfig::notifyAll(const StateVariant& event) {
    {
        std::lock_guard<decltype(pendingNotificationMutex)> lock(
            pendingNotificationMutex);
        pendingNotifications.push_back({true, 0, event});
        if (notifyingObservers) {
            return;
        }
        notifyingObservers = true;
    }
    dispatchPendingNotifications();
}

void ProjectConfig::notify(uint64_t observerKey, const StateVariant& event) {
    {
        std::lock_guard<decltype(pendingNotificationMutex)> lock(
            pendingNotificationMutex);
        pendingNotifications.push_back({false, observerKey, event});
        if (notifyingObservers) {
            return;
        }
        notifyingObservers = true;
    }
    dispatchPendingNotifications();
}

void ProjectConfig::dispatchPendingNotifications() {
    while (true) {
        PendingNotification notification;
        {
            std::lock_guard<decltype(pendingNotificationMutex)> lock(
                pendingNotificationMutex);
            if (pendingNotifications.empty()) {
                notifyingObservers = false;
                return;
            }
            notification = pendingNotifications.front();
            pendingNotifications.pop_front();
        }

        if (notification.allObservers) {
            Helpers::ISubject<StateVariant>::notifyAll(notification.event);
        } else {
            Helpers::ISubject<StateVariant>::notify(notification.observerKey,
                                                     notification.event);
        }
    }
}

ProjectConfig::SnapshotLoadResult ProjectConfig::loadSnapshot(
    config_snapshot& snapshot) {
    if (!isKey(kSnapshotKey)) {
        return SnapshotLoadResult::Absent;
    }
    if (isKey(kSnapshotAuthorityKey) &&
        getString(kSnapshotAuthorityKey, "") != kSnapshotAuthorityValue) {
        return SnapshotLoadResult::Invalid;
    }

    const String serializedSnapshot = getString(kSnapshotKey, "");
    return serializedSnapshot.length() > 0 &&
                   parseSnapshot(std::string(serializedSnapshot.c_str()),
                                 snapshot)
               ? SnapshotLoadResult::Valid
               : SnapshotLoadResult::Invalid;
}

void ProjectConfig::loadLegacyConfig() {
    // Start from defaults so repeated loads cannot retain stale snapshot data.
    this->config = Project_Config::ProjectConfig_t();
    this->config.mdns.hostname = _mdnsName;

    /* MDNS Config */
    const std::string storedMdns =
        getString(this->config.mdns.keys.hostname.c_str(), _mdnsName.c_str())
            .c_str();
    this->config.mdns.hostname =
        validHostname(storedMdns) ? storedMdns : _mdnsName;

    /* Device Config */
    this->config.device.ota_login.assign(
        getString(this->config.device.keys.ota_login.c_str(), "").c_str());
    this->config.device.ota_password.assign(
        getString(this->config.device.keys.ota_password.c_str(), "").c_str());
    const int storedOtaPort =
        getInt(this->config.device.keys.ota_port.c_str(), kDefaultOtaPort);
    this->config.device.ota_port =
        validOtaPort(storedOtaPort) ? storedOtaPort : kDefaultOtaPort;
    if (this->config.device.ota_login.empty() !=
        this->config.device.ota_password.empty()) {
        log_w("Partial legacy management credentials found; clearing them");
        this->config.device.ota_login.clear();
        this->config.device.ota_password.clear();
    }
    this->config.device_data.deviceJson.assign(
        getString(this->config.device_data.keys.deviceJson.c_str(), "")
            .c_str());

    // Never retain the legacy credentials shipped by older releases.
    if (this->config.device.ota_login == "admin" &&
        this->config.device.ota_password == "12345678") {
        log_w("Legacy OTA credentials found; clearing them");
        this->config.device.ota_login.clear();
        this->config.device.ota_password.clear();
        putString(this->config.device.keys.ota_login.c_str(), "");
        putString(this->config.device.keys.ota_password.c_str(), "");
    }

    /* Wifi TX Power Config */
    const uint32_t storedWifiPower =
        getUInt(this->config.wifKeys.power.c_str(), kDefaultWifiPower);
    this->config.wifi_tx_power.power =
        storedWifiPower <= 255 &&
                validWifiPower(static_cast<uint8_t>(storedWifiPower))
            ? static_cast<uint8_t>(storedWifiPower)
            : kDefaultWifiPower;

    /* WiFi Config */
    int networkCount = getInt("networkCount", 0);
    networkCount = std::max(0, std::min(networkCount, 3));

    for (int i = 0; i < networkCount; i++) {
        char buffer[12];
        std::string iter_str = Helpers::itoa(i, buffer, 10);

        const std::string name = this->config.wifKeys.name + iter_str;
        const std::string ssid = this->config.wifKeys.ssid + iter_str;
        const std::string password = this->config.wifKeys.password + iter_str;
        const std::string channel = this->config.wifKeys.channel + iter_str;
        const std::string power = this->config.wifKeys.power + iter_str;
        const std::string temp_1 = getString(name.c_str()).c_str();
        const std::string temp_2 = getString(ssid.c_str()).c_str();
        const std::string temp_3 = getString(password.c_str()).c_str();
        const uint32_t storedChannel = getUInt(channel.c_str());
        const uint32_t storedPower = getUInt(power.c_str());
        const uint8_t temp_4 =
            storedChannel <= 255 &&
                    validWifiChannel(static_cast<uint8_t>(storedChannel))
                ? static_cast<uint8_t>(storedChannel)
                : kDefaultWifiChannel;
        const uint8_t temp_5 =
            storedPower <= 255 &&
                    validWifiPower(static_cast<uint8_t>(storedPower))
                ? static_cast<uint8_t>(storedPower)
                : kDefaultWifiPower;

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
        getString(this->config.ap_network.keys.ssid.c_str(), _mdnsName.c_str())
            .c_str());
    this->config.ap_network.password.assign(
        getString(this->config.ap_network.keys.password.c_str(), "").c_str());
    const uint32_t storedApChannel =
        getUInt(this->config.ap_network.keys.channel.c_str(),
                kDefaultWifiChannel);
    this->config.ap_network.channel =
        storedApChannel <= 255 &&
                validWifiChannel(static_cast<uint8_t>(storedApChannel))
            ? static_cast<uint8_t>(storedApChannel)
            : kDefaultWifiChannel;

    if (this->config.ap_network.password == "12345678") {
        log_w("Legacy AP password found; clearing it");
        this->config.ap_network.password.clear();
        putString(this->config.ap_network.keys.password.c_str(), "");
    }
}

void ProjectConfig::load() {
    log_d("Loading project config");

    if (this->_already_loaded) {
        log_w("Project config already loaded");
        return;
    }

    initConfig();

    /* Custom Load */
    if (_custom_config_interface != nullptr)
        _custom_config_interface->load();

    config_snapshot snapshot;
    switch (loadSnapshot(snapshot)) {
        case SnapshotLoadResult::Valid:
            this->config.device = std::move(snapshot.device);
            this->config.device_data = std::move(snapshot.device_data);
            this->config.mdns = std::move(snapshot.mdns);
            this->config.wifi_tx_power = std::move(snapshot.wifi_tx_power);
            this->config.ap_network = std::move(snapshot.ap_network);
            this->config.networks = std::move(snapshot.networks);
            persistenceAuthority = ConfigPersistenceAuthority::Snapshot;
            persistenceFailed = false;
            break;
        case SnapshotLoadResult::Absent:
            loadLegacyConfig();
            if (persistSnapshot()) {
                persistenceAuthority = ConfigPersistenceAuthority::Snapshot;
                persistenceFailed = false;
            } else {
                persistenceAuthority = ConfigPersistenceAuthority::Legacy;
                persistenceFailed = true;
                log_e("Legacy project config migration failed");
            }
            break;
        case SnapshotLoadResult::Invalid:
            // A present snapshot is authoritative even when it is unusable;
            // never fall back to potentially stale legacy credentials.
            this->config = Project_Config::ProjectConfig_t();
            this->config.mdns.hostname = _mdnsName;
            persistenceAuthority =
                ConfigPersistenceAuthority::InvalidSnapshot;
            persistenceFailed = false;
            log_e("Project config snapshot is invalid or unsupported");
            break;
    }

    this->_already_loaded = true;
    this->notifyAll(Event_e::configLoaded);
}

//**********************************************************************************************************************
//*
//!                                                Setters
//*
//**********************************************************************************************************************
void ProjectConfig::setDeviceConfig(const std::string& ota_pass, int ota_port,
                                     bool shouldNotify) {
    setDeviceConfig(this->config.device.ota_login, ota_pass, ota_port,
                    shouldNotify);
}

void ProjectConfig::setDeviceConfig(const std::string& ota_login,
                                    const std::string& ota_pass, int ota_port,
                                    bool shouldNotify) {
    log_d("Updating device config");
    if (!validOtaPort(ota_port)) {
        log_e("Invalid OTA port: %d", ota_port);
        return;
    }
    this->config.device.ota_login.assign(ota_login);
    this->config.device.ota_password.assign(ota_pass);
    this->config.device.ota_port = ota_port;

    if (shouldNotify) {
        this->notifyAll(Event_e::deviceConfigUpdated);
    }
}

bool ProjectConfig::setMDNSConfig(const std::string& hostname,
                                  bool shouldNotify) {
    log_d("Updating MDNS config");
    for (char character : hostname) {
        if (character == '-' || character == '.')
            continue;
        else if (character >= '0' && character <= '9')
            continue;
        else if (character >= 'A' && character <= 'Z')
            continue;
        else if (character >= 'a' && character <= 'z')
            continue;
        log_i(
            "Invalid hostname, please use only alphanumeric "
            "characters");
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
                                  uint8_t power, bool adhoc, bool shouldNotify,
                                  bool shouldReboot) {
    if (!validWifiChannel(channel) || !validWifiPower(power) ||
        this->config.networks.size() > kMaxSavedNetworks) {
        log_e("Invalid WiFi configuration or network limit exceeded");
        return;
    }

    // we store the ADHOC flag as false because the networks we store in the
    // config are the ones we want the esp to connect to, rather than host as
    // AP, and here we're just updating them
    size_t size = this->config.networks.size();
    auto wifiHandler = static_cast<uint64_t>(
        ProjectConfigEventIDs_e::ProjectConfigEventID_WifiHandler);

    for (auto it = this->config.networks.begin();
         it != this->config.networks.end();) {
        if (it->ssid == ssid) {
            log_i("Found network %s, updating it ...", it->name.c_str());

            it->name = networkName;
            it->ssid = ssid;
            it->password = password;
            it->channel = channel;
            it->power = power;
            it->adhoc = false;

            if (shouldNotify) {
                this->notify(wifiHandler, WiFiState_e::WiFiState_Disconnected);
                WiFi.disconnect();
                this->wifiConfigSave();
                this->notify(wifiHandler, Event_e::networksConfigUpdated);
            }

            return;
        } else {
            ++it;
        }
    }

    if (size < 3 && size > 0) {
        this->log("We're adding a new network");
        // we don't have that network yet, we can add it as we still have some
        // space we're using emplace_back as push_back will create a copy of it,
        // we want to avoid that
        this->config.networks.emplace_back(networkName, ssid, password, channel,
                                           power, false);
    }

    // we're allowing to store up to three additional networks
    if (size == 0) {
        this->log("No networks, We're adding a new network");
        this->config.networks.emplace_back(networkName, ssid, password, channel,
                                           power, false);
    }

    if (shouldNotify) {
        this->notify(wifiHandler, WiFiState_e::WiFiState_Disconnected);
        WiFi.disconnect();
        this->wifiConfigSave();
        this->notify(wifiHandler, Event_e::networksConfigUpdated);
    }

    if (shouldReboot) {
        this->reboot = true;
    }
}

void ProjectConfig::deleteWifiConfig(const std::string& networkName,
                                     bool shouldNotify) {
    size_t size = this->config.networks.size();
    if (size == 0) {
        this->log("No networks, nothing to delete");
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
    if (!validWifiChannel(channel)) {
        log_e("Invalid access point channel: %u", channel);
        return;
    }
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
    if (!validWifiPower(power)) {
        log_e("Invalid WiFi TX power: %u", power);
        return;
    }
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
        "\"device_config\": {\"ota_login\": \"%s\",\"ota_pass\": null, "
        "\"ota_port\": %u}",
        jsonEscape(this->ota_login).c_str(), this->ota_port);
    return json;
}

std::string Project_Config::MDNSConfig_t::toRepresentation() {
    std::string json = Helpers::format_string(
        "\"mdns_config\": {\"hostname\": \"%s\"}",
        jsonEscape(this->hostname).c_str());
    return json;
}

std::string Project_Config::WiFiConfig_t::toRepresentation() {
    std::string json = Helpers::format_string(
        "{\"name\": \"%s\", \"ssid\": \"%s\", \"password\": null, "
        "\"channel\": %u, \"power\": %u, \"adhoc\": %s}",
        jsonEscape(this->name).c_str(), jsonEscape(this->ssid).c_str(),
        this->channel, this->power, this->adhoc ? "true" : "false");
    return json;
}

std::string Project_Config::AP_WiFiConfig_t::toRepresentation() {
    std::string json = Helpers::format_string(
        "\"ap_wifi_config\": {\"ssid\": \"%s\", \"password\": null, "
        "\"channel\": %u, \"adhoc\": %s}",
        jsonEscape(this->ssid).c_str(), this->channel,
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
        "\"deviceData\": {\"data\": \"%s\"}",
        jsonEscape(this->deviceJson).c_str());
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

//**********************************************************************************************************************
//*
//!                                                callbacks
//*
//**********************************************************************************************************************

void ProjectConfig::registerUserConfig(
    _custom_config_interface_t custom_config_interface) {
    this->_custom_config_interface = custom_config_interface;
}
