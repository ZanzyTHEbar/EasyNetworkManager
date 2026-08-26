#ifndef OTA_HPP
#define OTA_HPP
#include <ArduinoOTA.h>

#include <string>

#include "data/config/project_config.hpp"

class OTA {
   private:
    unsigned long _bootTimestamp;
    bool _isOtaEnabled;
    std::string _otaPassword;
    ProjectConfig& _deviceConfig;

   public:
    OTA(ProjectConfig& deviceConfig);
    virtual ~OTA();

    void begin();
    void handleOTAUpdate();
};
#endif  // OTA_HPP
