#pragma once
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
              std::string service_name,
              std::string service_text,
              std::string proto,
              std::string key,
              std::string value);
  void startMDNS();
  void update(ObserverEvent::Event event);
};