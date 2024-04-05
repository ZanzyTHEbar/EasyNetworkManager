#pragma once
#include <data/config/project_config.hpp>
#include <data/config/states.hpp>
#include <helpers/logger.hpp>
#include <helpers/observer.hpp>

class ConfigHandler : public Helpers::Logger,
                      public Helpers::IObserver<StateVariant> {
   public:
    ConfigHandler(const std::string& configName = std::string(),
                  const std::string& mdnsName = std::string());
    virtual ~ConfigHandler();
    virtual void begin();
    virtual void update(const StateVariant& event) override;
    ProjectConfig config;
};