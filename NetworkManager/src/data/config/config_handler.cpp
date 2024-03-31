#include <data/config/config_handler.hpp>

ConfigHandler::ConfigHandler(const std::string& configName,
                             const std::string& mdnsName)
    : config(std::move(configName), std::move(mdnsName)) {}

ConfigHandler::~ConfigHandler() {}

void ConfigHandler::begin() {
    config.load();
}

void ConfigHandler::update(const StateVariant& event) {
    updateWrapper<Event_e>(event, [this](Event_e _event) {
        switch (_event) {
            case Event_e::configSaved:
                this->begin();
                break;
            default:
                break;
        }
    });
}
