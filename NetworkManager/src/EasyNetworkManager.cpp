#include <EasyNetworkManager.hpp>

EasyNetworkManager::EasyNetworkManager(
    const std::string& config_name, const std::string& hostname,
    const std::string& ssid, const std::string& password, int channel,
    const std::string& service_name, const std::string& service_instance_name,
    const std::string& service_protocol, const std::string& service_description,
    const std::string& service_port, bool enable_mdns, bool enable_adhoc)
    : configHandler(std::make_shared<ConfigHandler>(std::move(config_name),
                                                    std::move(hostname))),
      wifiHandler(std::make_shared<WiFiHandler>(configHandler->config,
                                                std::move(ssid),
                                                std::move(password), channel)) {
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
    wifiHandler->begin();

    if (mdnsHandler != nullptr)
        mdnsHandler->begin();
}