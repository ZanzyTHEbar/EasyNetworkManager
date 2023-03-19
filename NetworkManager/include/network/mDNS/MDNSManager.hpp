#ifndef MDNS_HANDLER_HPP
#define MDNS_HANDLER_HPP
#include <ESPmDNS.h>

#include "data/config/project_config.hpp"
#include "data/statemanager/StateManager.hpp"
#include "utilities/Observer.hpp"

class MDNSHandler : public IObserver<Event_e> {
   private:
    ProjectConfig& configManager;
    std::string service_name;
    std::string service_text;
    std::string proto;
    std::string key;
    std::string value;

    std::string name;

   public:
    MDNSHandler(ProjectConfig& configManager, const std::string& service_name,
                const std::string& service_text, const std::string& proto,
                const std::string& key, const std::string& value);
    bool begin();
    void setName(const std::string& name);
    void update(Event_e event) override;
    std::string getName() override;
};

#endif  // MDNS_HANDLER_HPP
