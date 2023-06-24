#include "data/config/config_handler.hpp"

ConfigHandler::ConfigHandler(const std::string& configName,
                             const std::string& mdnsName)
    : config(std::move(configName), std::move(mdnsName)) {}

ConfigHandler::~ConfigHandler() {}

void ConfigHandler::begin() {
    config.load();
}

void ConfigHandler::update(Event_e event) {
    switch (event) {
        case Event_e::configSaved:
            this->begin();
            break;
        default:
            break;
    }
}

std::string ConfigHandler::getName() {
    return "ConfigHandler";
}