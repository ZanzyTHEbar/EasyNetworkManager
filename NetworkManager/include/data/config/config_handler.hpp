#pragma once
#include <data/config/project_config.hpp>
#include <helpers/observer.hpp>
#include <utilities/states.hpp>

class ConfigHandler : public Helpers::IObserver<StateVariant> {
   public:
    ConfigHandler(const std::string& configName = std::string(),
                  const std::string& mdnsName = std::string());
    virtual ~ConfigHandler();
    virtual void begin();
    virtual void update(const StateVariant& event) override;
    std::string getName() const override {
        return "ConfigHandler";
    }
    ProjectConfig config;
};