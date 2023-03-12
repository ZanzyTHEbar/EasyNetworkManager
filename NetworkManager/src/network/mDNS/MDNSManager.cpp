#include <network/mDNS/MDNSManager.hpp>

MDNSHandler::MDNSHandler(StateManager<MDNSState_e>* stateManager,
                         ProjectConfig* configManager,
                         const std::string& service_name,
                         const std::string& service_text,
                         const std::string& proto, const std::string& key,
                         const std::string& value)
    : stateManager(stateManager),
      configManager(configManager),
      service_name(service_name),
      service_text(service_text),
      proto(proto),
      key(key),
      value(value) {}

bool MDNSHandler::begin() {
    auto mdnsConfig = configManager->getMDNSConfig();
    log_d("%s", mdnsConfig->hostname.c_str());
    if (!MDNS.begin(mdnsConfig->hostname.c_str())) {
        stateManager->setState(MDNSState_e::MDNSState_Error);
        log_e("Error initializing MDNS");
        return false;
    }

    stateManager->setState(MDNSState_e::MDNSState_Starting);
    MDNS.addService(service_name.c_str(), proto.c_str(), atoi(value.c_str()));
    MDNS.addServiceTxt(
        service_name.c_str(), proto.c_str(), key.c_str(),
        value.c_str());  // ex: "camera", "tcp", "stream_port", "80"
    log_d("%s %s %s %s %s", service_name.c_str(), proto.c_str(),
          service_text.c_str(), key.c_str(), value.c_str());
    log_i("MDNS initialized!");
    stateManager->setState(MDNSState_e::MDNSState_Started);
    return true;
}

void MDNSHandler::update(Event_e event) {
    switch (event) {
        case Event_e::mdnsConfigUpdated:
            MDNS.end();
            begin();
            break;
        default:
            break;
    }
}

std::string MDNSHandler::getName() {
    return std::string();
}

void MDNSHandler::setName(const std::string& name) {
    this->name.assign(name);
}
