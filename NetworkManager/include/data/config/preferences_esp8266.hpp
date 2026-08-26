#pragma once

#if defined(ESP8266)

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

// Minimal Preferences-compatible storage for the ESP8266 path. The values are
// kept in one small JSON object and each update replaces the file through a
// temporary file before renaming it.
class Preferences {
   public:
    Preferences() = default;

    bool begin(const char* name, bool readOnly = false);
    void end();
    bool isKey(const char* key) const;

    size_t putString(const char* key, const char* value);
    String getString(const char* key, const char* defaultValue = "") const;
    size_t putInt(const char* key, int32_t value);
    int32_t getInt(const char* key, int32_t defaultValue = 0) const;
    size_t putUInt(const char* key, uint32_t value);
    uint32_t getUInt(const char* key, uint32_t defaultValue = 0) const;
    bool clear();

   private:
    static String makePath(const char* name);
    static bool loadObject(const String& path, JsonDocument& values);
    bool recoverStorage();
    static bool removeIfPresent(const String& path);
    bool restoreBackup(bool hadTarget);
    bool persist();

    JsonDocument values;
    String path;
    String temporaryPath;
    String backupPath;
    bool started = false;
    bool readOnly = false;
};

#endif  // ESP8266
