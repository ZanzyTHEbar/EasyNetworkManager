#ifndef MDNS_HANDLER_HPP
#define MDNS_HANDLER_HPP
#include <ESPmDNS.h>

#include "data/config/project_config.hpp"
#include "data/statemanager/StateManager.hpp"
#include "utilities/Observer.hpp"

class MDNSHandler : public IObserver<ObserverEvent::Event> {
   private:
    StateManager<MDNSState_e>* stateManager;
    ProjectConfig* configManager;
    std::string service_name;
    std::string service_text;
    std::string proto;
    std::string key;
    std::string value;

   public:
    MDNSHandler(StateManager<MDNSState_e>* stateManager,
                ProjectConfig* configManager, const std::string& service_name,
                const std::string& service_text, const std::string& proto,
                const std::string& key, const std::string& value);
    bool begin();
    void update(ObserverEvent::Event event);
};

#endif  // MDNS_HANDLER_HPP
