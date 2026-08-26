#include <EasyNetworkManager.hpp>

EasyNetworkManager::EasyNetworkManager(
    const std::string& config_name, const std::string& hostname,
    const std::string& ssid, const std::string& password, int channel,
     const std::string& service_name, const std::string& service_instance_name,
     const std::string& service_protocol, const std::string& service_description,
     const std::string& service_port, bool enable_mdns, bool enable_adhoc,
     const std::string& management_login,
     const std::string& management_password, const std::string& ap_ssid,
     const std::string& ap_password)
    : configHandler(std::make_shared<ConfigHandler>(std::move(config_name),
                                                     std::move(hostname))),
      wifiHandler(std::make_shared<WiFiHandler>(configHandler->config,
                                                 std::move(ssid),
                                                 std::move(password), channel)),
      management_login(management_login),
      management_password(management_password),
      ap_ssid(ap_ssid),
      ap_password(ap_password),
      enable_adhoc(enable_adhoc) {
    if (enable_mdns) {
        mdnsHandler = std::make_shared<MDNSHandler>(
            configHandler->config, std::move(service_name),
            std::move(service_instance_name), std::move(service_protocol),
            std::move(service_description), std::move(service_port));
    }
}

EasyNetworkManager::~EasyNetworkManager() {}

void EasyNetworkManager::begin() {
    configHandler->config.attach(configHandler);
    configHandler->config.attach(wifiHandler);
    if (mdnsHandler != nullptr)
    configHandler->config.attach(mdnsHandler);

    configHandler->begin();

    bool seededConfig = false;
    {
        const auto& deviceConfig = configHandler->config.getDeviceConfig();
        if (!management_login.empty() && !management_password.empty() &&
            deviceConfig.ota_login.empty() &&
            deviceConfig.ota_password.empty()) {
            configHandler->config.setDeviceConfig(
                management_login, management_password, deviceConfig.ota_port,
                false);
            seededConfig = true;
        }

        const auto& apConfig = configHandler->config.getAPWifiConfig();
        // The /provision gate requires a strong non-default AP password;
        // seeding anything weaker would close first-boot permanently.
        if (!ap_ssid.empty() && apConfig.password.empty()) {
            if (ap_password.length() >= 8 && ap_password != "12345678") {
                configHandler->config.setAPWifiConfig(
                    ap_ssid, ap_password, apConfig.channel, apConfig.adhoc,
                    false);
                seededConfig = true;
            } else {
                log_e("Supplied AP password is missing strength (>=8 chars, "
                      "not the legacy default); AP bootstrap disabled");
            }
        }
    }
    if (seededConfig) {
        configHandler->config.save();
        if (!configHandler->config.lastSaveSucceeded()) {
            log_e("Constructor configuration seed could not be persisted");
        }
    }

    wifiHandler->toggleAdhoc(enable_adhoc);
    wifiHandler->begin();

    if (mdnsHandler != nullptr)
        mdnsHandler->begin();
}
