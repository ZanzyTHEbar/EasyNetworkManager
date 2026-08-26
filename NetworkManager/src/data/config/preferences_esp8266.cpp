#include <data/config/preferences_esp8266.hpp>

#if defined(ESP8266)

#include <cstring>
#include <utility>

namespace {

bool removeIfPresent(const String& path) {
    return !LittleFS.exists(path.c_str()) || LittleFS.remove(path.c_str());
}

bool hasString(JsonObjectConst object, const char* key) {
    const JsonVariantConst value = object[key];
    return value.is<const char*>() && value.as<const char*>() != nullptr;
}

bool hasObject(JsonObjectConst object, const char* key,
               JsonObjectConst& value) {
    const JsonVariantConst candidate = object[key];
    if (!candidate.is<JsonObjectConst>()) {
        return false;
    }
    value = candidate.as<JsonObjectConst>();
    return true;
}

bool validHostname(JsonObjectConst object, const char* key) {
    const char* hostname = object[key].as<const char*>();
    if (hostname == nullptr) {
        return false;
    }
    for (const char* character = hostname; *character != '\0'; ++character) {
        if ((*character >= '0' && *character <= '9') ||
            (*character >= 'A' && *character <= 'Z') ||
            (*character >= 'a' && *character <= 'z') || *character == '-' ||
            *character == '.') {
            continue;
        }
        return false;
    }
    return true;
}

bool validSnapshotCandidate(const JsonDocument& values) {
    const JsonObjectConst storage = values.as<JsonObjectConst>();
    if (storage.containsKey("config_snapshot_authority")) {
        const JsonVariantConst authority =
            storage["config_snapshot_authority"];
        if (!authority.is<const char*>() ||
            std::strcmp(authority.as<const char*>(), "snapshot-v1") != 0) {
            return false;
        }
    }

    const JsonVariantConst serialized = values["config_snapshot"];
    if (!serialized.is<const char*>()) {
        return false;
    }

    JsonDocument snapshot;
    if (deserializeJson(snapshot, serialized.as<const char*>()) ||
        !snapshot.is<JsonObjectConst>()) {
        return false;
    }

    const JsonObjectConst root = snapshot.as<JsonObjectConst>();
    const JsonVariantConst version = root["version"];
    if (!version.is<uint8_t>() || version.as<uint8_t>() != 1) {
        return false;
    }

    JsonObjectConst device;
    if (!hasObject(root, "device", device)) {
        return false;
    }
    const JsonVariantConst otaPort = device["ota_port"];
    if (!hasString(device, "ota_login") ||
        !hasString(device, "ota_password") || !otaPort.is<int>() ||
        otaPort.as<int>() < 1 || otaPort.as<int>() > 65535) {
        return false;
    }

    JsonObjectConst deviceData;
    if (!hasObject(root, "device_data", deviceData) ||
        !hasString(deviceData, "deviceJson")) {
        return false;
    }

    JsonObjectConst mdns;
    if (!hasObject(root, "mdns", mdns) || !hasString(mdns, "hostname") ||
        !validHostname(mdns, "hostname")) {
        return false;
    }

    JsonObjectConst txPower;
    if (!hasObject(root, "wifi_tx_power", txPower)) {
        return false;
    }
    const JsonVariantConst power = txPower["power"];
    if (!power.is<uint8_t>() || power.as<uint8_t>() > 78) {
        return false;
    }

    JsonObjectConst apNetwork;
    if (!hasObject(root, "ap_network", apNetwork)) {
        return false;
    }
    const JsonVariantConst apChannel = apNetwork["channel"];
    if (!hasString(apNetwork, "ssid") ||
        !hasString(apNetwork, "password") || !apChannel.is<uint8_t>() ||
        apChannel.as<uint8_t>() < 1 || apChannel.as<uint8_t>() > 14 ||
        !apNetwork["adhoc"].is<bool>()) {
        return false;
    }

    const JsonVariantConst networkCount = root["network_count"];
    JsonArrayConst networks;
    if (!networkCount.is<uint8_t>() || networkCount.as<uint8_t>() > 3 ||
        !root["networks"].is<JsonArrayConst>()) {
        return false;
    }
    networks = root["networks"].as<JsonArrayConst>();
    if (networks.size() != networkCount.as<uint8_t>()) {
        return false;
    }

    for (JsonVariantConst candidate : networks) {
        if (!candidate.is<JsonObjectConst>()) {
            return false;
        }
        const JsonObjectConst network = candidate.as<JsonObjectConst>();
        const JsonVariantConst channel = network["channel"];
        const JsonVariantConst networkPower = network["power"];
        if (!hasString(network, "name") || !hasString(network, "ssid") ||
            !hasString(network, "password") || !channel.is<uint8_t>() ||
            channel.as<uint8_t>() < 1 || channel.as<uint8_t>() > 14 ||
            !networkPower.is<uint8_t>() || networkPower.as<uint8_t>() > 78 ||
            !network["adhoc"].is<bool>()) {
            return false;
        }
    }

    return true;
}

bool validStorageCandidate(const JsonDocument& values) {
    const JsonObjectConst object = values.as<JsonObjectConst>();
    return object && (object.size() == 0 || validSnapshotCandidate(values));
}

}  // namespace

String Preferences::makePath(const char* name) {
    String result = "/enm_";
    if (name != nullptr) {
        for (const char* character = name; *character != '\0'; ++character) {
            if ((*character >= 'a' && *character <= 'z') ||
                (*character >= 'A' && *character <= 'Z') ||
                (*character >= '0' && *character <= '9') ||
                *character == '_' || *character == '-') {
                result += *character;
            } else {
                result += '_';
            }
        }
    }
    result += ".json";
    return result;
}

bool Preferences::loadObject(const String& path, JsonDocument& values) {
    File file = LittleFS.open(path.c_str(), "r");
    if (!file) {
        return false;
    }

    JsonDocument parsed;
    const DeserializationError error = deserializeJson(parsed, file);
    file.close();
    if (error || !parsed.is<JsonObject>()) {
        return false;
    }

    values = std::move(parsed);
    return true;
}

bool Preferences::removeIfPresent(const String& path) {
    return ::removeIfPresent(path);
}

bool Preferences::recoverStorage() {
    const bool targetExists = LittleFS.exists(path.c_str());
    const bool backupExists = LittleFS.exists(backupPath.c_str());

    JsonDocument target;
    const bool targetValid = targetExists && loadObject(path, target);
    const bool targetHasSnapshot =
        targetValid &&
        target.as<JsonObjectConst>().containsKey("config_snapshot");
    const bool targetSnapshotValid =
        targetValid && validSnapshotCandidate(target);
    JsonDocument backup;
    const bool backupValid = backupExists && loadObject(backupPath, backup);
    const bool backupSnapshotValid =
        backupValid && validSnapshotCandidate(backup);
    bool backupConsumed = false;

    if ((!targetValid || (targetHasSnapshot && !targetSnapshotValid)) &&
        backupSnapshotValid) {
        if (targetExists && !removeIfPresent(path)) {
            return false;
        }
        if (!LittleFS.rename(backupPath.c_str(), path.c_str())) {
            return false;
        }

        JsonDocument restored;
        if (!loadObject(path, restored) || !validSnapshotCandidate(restored)) {
            return false;
        }
        backupConsumed = true;
    }

    // A backup without a valid snapshot is not a recovery candidate. A valid
    // backup stays in place until a replacement has been validated.
    if (backupExists && !backupConsumed && !backupSnapshotValid) {
        if (!removeIfPresent(backupPath)) {
            return false;
        }
    }

    return removeIfPresent(temporaryPath);
}

bool Preferences::begin(const char* name, bool readOnlyValue) {
    started = false;
    readOnly = readOnlyValue;
    path = makePath(name);
    temporaryPath = path + ".tmp";
    backupPath = path + ".bak";
    values.clear();

    if (!LittleFS.begin()) {
        return false;
    }

    if (!recoverStorage()) {
        return false;
    }

    if (!loadObject(path, values)) {
        values.clear();
        values.to<JsonObject>();
    }

    started = true;
    return true;
}

void Preferences::end() {
    started = false;
}

bool Preferences::isKey(const char* key) const {
    return started && key != nullptr && !values[key].isNull();
}

bool Preferences::persist() {
    if (!started || readOnly) {
        return false;
    }

    if (!recoverStorage()) {
        return false;
    }

    File file = LittleFS.open(temporaryPath.c_str(), "w");
    if (!file) {
        return false;
    }

    const size_t written = serializeJson(values, file);
    file.flush();
    file.close();
    if (written == 0) {
        removeIfPresent(temporaryPath);
        return false;
    }

    JsonDocument temporaryValues;
    if (!loadObject(temporaryPath, temporaryValues) ||
        !validStorageCandidate(temporaryValues)) {
        removeIfPresent(temporaryPath);
        return false;
    }

    const bool hadTarget = LittleFS.exists(path.c_str());
    JsonDocument backupValues;
    const bool backupExists = LittleFS.exists(backupPath.c_str());
    const bool backupValid =
        backupExists && loadObject(backupPath, backupValues) &&
        validSnapshotCandidate(backupValues);

    if (backupExists && !backupValid && !removeIfPresent(backupPath)) {
        removeIfPresent(temporaryPath);
        return false;
    }

    if (hadTarget) {
        if (backupValid) {
            // Keep the last valid backup until the new target has been
            // installed and checked.
            if (!removeIfPresent(path)) {
                removeIfPresent(temporaryPath);
                return false;
            }
        } else if (!LittleFS.rename(path.c_str(), backupPath.c_str())) {
            removeIfPresent(temporaryPath);
            return false;
        }
    }

    if (!LittleFS.rename(temporaryPath.c_str(), path.c_str())) {
        if (LittleFS.exists(backupPath.c_str())) {
            restoreBackup(true);
        }
        removeIfPresent(temporaryPath);
        return false;
    }

    JsonDocument persistedValues;
    if (!loadObject(path, persistedValues) ||
        !validStorageCandidate(persistedValues)) {
        removeIfPresent(path);
        if (LittleFS.exists(backupPath.c_str())) {
            restoreBackup(true);
        }
        removeIfPresent(temporaryPath);
        return false;
    }

    if (!removeIfPresent(backupPath) || !removeIfPresent(temporaryPath)) {
        return false;
    }
    return true;
}

bool Preferences::restoreBackup(bool hadTarget) {
    if (!LittleFS.exists(backupPath.c_str())) {
        return !hadTarget;
    }
    if (!removeIfPresent(path)) {
        return false;
    }
    if (!LittleFS.rename(backupPath.c_str(), path.c_str())) {
        return false;
    }

    JsonDocument restoredValues;
    return loadObject(path, restoredValues) &&
           validStorageCandidate(restoredValues);
}

size_t Preferences::putString(const char* key, const char* value) {
    if (!started || readOnly || key == nullptr || value == nullptr) {
        return 0;
    }
    values[key] = value;
    return persist() ? strlen(value) + 1 : 0;
}

String Preferences::getString(const char* key, const char* defaultValue) const {
    if (!started || key == nullptr) {
        return String(defaultValue == nullptr ? "" : defaultValue);
    }

    JsonVariantConst value = values[key];
    const char* storedValue = value.as<const char*>();
    return storedValue == nullptr
               ? String(defaultValue == nullptr ? "" : defaultValue)
               : String(storedValue);
}

size_t Preferences::putInt(const char* key, int32_t value) {
    if (!started || readOnly || key == nullptr) {
        return 0;
    }
    values[key] = value;
    return persist() ? sizeof(value) : 0;
}

int32_t Preferences::getInt(const char* key, int32_t defaultValue) const {
    if (!started || key == nullptr) {
        return defaultValue;
    }

    JsonVariantConst value = values[key];
    return value.is<int32_t>() ? value.as<int32_t>() : defaultValue;
}

size_t Preferences::putUInt(const char* key, uint32_t value) {
    if (!started || readOnly || key == nullptr) {
        return 0;
    }
    values[key] = value;
    return persist() ? sizeof(value) : 0;
}

uint32_t Preferences::getUInt(const char* key, uint32_t defaultValue) const {
    if (!started || key == nullptr) {
        return defaultValue;
    }

    JsonVariantConst value = values[key];
    return value.is<uint32_t>() ? value.as<uint32_t>() : defaultValue;
}

bool Preferences::clear() {
    if (!started || readOnly) {
        return false;
    }
    values.clear();
    values.to<JsonObject>();
    return persist();
}

#endif  // ESP8266
