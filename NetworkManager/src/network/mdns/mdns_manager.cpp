#include <network/mdns/mdns_manager.hpp>

MDNSHandler::MDNSHandler(ProjectConfig& configManager,
                         const std::string& service_name,
                         const std::string& service_text,
                         const std::string& proto, const std::string& key,
                         const std::string& value)
    : configManager(configManager),
      service_name(std::move(service_name)),
      service_text(std::move(service_text)),
      proto(std::move(proto)),
      key(std::move(key)),
      value(std::move(value)) {}

bool MDNSHandler::begin() {
    auto mdnsConfig = configManager.getMDNSConfig();
    log_d("%s", mdnsConfig.hostname.c_str());
    if (!MDNS.begin(mdnsConfig.hostname.c_str())) {
        this->configManager.setState(this->getID(),
                                     MDNSState_e::MDNSState_Error);
        log_e("Error initializing MDNS");
        return false;
    }

    this->configManager.setState(this->getID(),
                                 MDNSState_e::MDNSState_Starting);
    MDNS.addService(service_name.c_str(), proto.c_str(), atoi(value.c_str()));
    MDNS.addServiceTxt(
        service_name.c_str(), proto.c_str(), key.c_str(),
        value.c_str());  // ex: "camera", "tcp", "stream_port", "80"
    log_d("%s %s %s %s %s", service_name.c_str(), proto.c_str(),
          service_text.c_str(), key.c_str(), value.c_str());
    log_i("MDNS initialized!");
    this->configManager.setState(this->getID(), MDNSState_e::MDNSState_Started);
    return true;
}

void MDNSHandler::update(const StateVariant& event) {
    updateWrapper<Event_e>(event, [this](Event_e _event) {
        switch (_event) {
            case Event_e::mdnsConfigUpdated:
                MDNS.end();
                this->begin();
                break;
            default:
                break;
        }
    });
}
