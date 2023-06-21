#pragma once
#include "data/config/project_config.hpp"
#include "data/statemanager/state_manager.hpp"
#include "utilities/observer.hpp"

class ConfigHandler : public IObserver<Event_e> {
   public:
    ConfigHandler(ProjectConfig& config);
    ~ConfigHandler();
    void begin();
    void update(Event_e event) override;
    std::string getName() override;

   private:
    ProjectConfig& config;
};