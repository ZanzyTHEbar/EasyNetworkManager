#ifndef MDNS_HANDLER_HPP
#define MDNS_HANDLER_HPP
#include <ESPmDNS.h>
#include "data/StateManager/StateManager.hpp"
#include "data/utilities/Observer.hpp"
#include "data/config/project_config.hpp"

class MDNSHandler : public IObserver
{
private:
  StateManager<ProgramStates::DeviceStates::MDNSState_e> *stateManager;
  ProjectConfig *configManager;
  std::string service_name;
  std::string service_text;
  std::string proto;
  std::string key;
  std::string value;

public:
  MDNSHandler(StateManager<ProgramStates::DeviceStates::MDNSState_e> *stateManager,
              ProjectConfig *configManager,
              const std::string &service_name,
              const std::string &service_text,
              const std::string &proto,
              const std::string &key,
              const std::string &value);
  bool startMDNS();
  void update(ObserverEvent::Event event);
  int DiscovermDNSBroker();
};

#endif // MDNS_HANDLER_HPP
