#pragma once
#include "data/config/project_config.hpp"
#include "data/statemanager/StateManager.hpp"
#include "utilities/Observer.hpp"

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