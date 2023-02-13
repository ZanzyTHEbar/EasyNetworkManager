#ifndef OTA_HPP
#define OTA_HPP
#include <ArduinoOTA.h>
#include <HTTPClient.h>

#include "data/config/project_config.hpp"

class OTA {
   public:
    OTA(ProjectConfig* deviceConfig);
    virtual ~OTA();

    void SetupOTA();
    void HandleOTAUpdate();

   private:
    unsigned long _bootTimestamp;
    bool _isOtaEnabled;
    ProjectConfig* _deviceConfig;
};
#endif  // OTA_HPP