#pragma once
#include <ESPmDNS.h>
#include <data/config/project_config.hpp>
#include <data/config/states.hpp>
#include <helpers/logger.hpp>
#include <helpers/observer.hpp>

class MDNSHandler : public Helpers::Logger,
                    public Helpers::IObserver<StateVariant> {
   private:
    ProjectConfig& configManager;
    std::string service_name;
    std::string service_text;
    std::string proto;
    std::string key;
    std::string value;

   public:
    MDNSHandler(ProjectConfig& configManager, const std::string& service_name,
                const std::string& service_text, const std::string& proto,
                const std::string& key, const std::string& value);
    bool begin();
    /* Overrides */
    void update(const StateVariant& event) override;
};
