#pragma once
#include "data/config/project_config.hpp"
#include "data/statemanager/state_manager.hpp"
#include "utilities/observer.hpp"

class ConfigHandler : public IObserver<Event_e> {
   public:
    ConfigHandler(const std::string& configName = std::string(),
                  const std::string& mdnsName = std::string());
    virtual ~ConfigHandler();
    virtual void begin();
    virtual void update(Event_e event) override;
    virtual std::string getName() override;
    ProjectConfig config;
};