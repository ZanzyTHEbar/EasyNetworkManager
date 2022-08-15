#include "MDNSManager.hpp"

MDNSHandler::MDNSHandler(StateManager<ProgramStates::DeviceStates::MDNSState_e> *stateManager,
                         ProjectConfig *configManager,
                         std::string service_name,
                         std::string service_text,
                         std::string proto,
                         std::string key,
                         std::string value) : stateManager(stateManager),
                                              configManager(configManager),
                                              service_name(service_name),
                                              service_text(service_text),
                                              proto(proto),
                                              key(proto),
                                              value(value) {}

void MDNSHandler::startMDNS()
{
  Project_Config::DeviceConfig_t *deviceConfig = configManager->getDeviceConfig();

  if (MDNS.begin(deviceConfig->name.c_str()))
  {
    stateManager->setState(MDNSState_e::MDNSState_Starting);
    MDNS.addService(service_name.c_str(), proto.c_str(), atoi(value.c_str()));
    MDNS.addServiceTxt(service_text.c_str(), proto.c_str(), key.c_str(), value.c_str()); // ex: "camera", "tcp", "stream_port", "80"
    log_i("MDNS initialized!");
    stateManager->setState(MDNSState_e::MDNSState_Started);
  }
  else
  {
    stateManager->setState(MDNSState_e::MDNSState_Error);
    log_e("Error initializing MDNS");
  }
}

void MDNSHandler::update(ObserverEvent::Event event)
{
  if (event == ObserverEvent::deviceConfigUpdated)
  {
    MDNS.end();
    startMDNS();
  }
}