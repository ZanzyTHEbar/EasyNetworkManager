#include <data/config/config_snapshot_codec.hpp>

#include <cstdio>
#include <utility>

namespace {

constexpr uint8_t kMaxWifiChannel = 14;
constexpr uint8_t kMaxWifiPower = 78;

bool readString(JsonObjectConst object, const char* key, std::string& value) {
    const JsonVariantConst jsonValue = object[key];
    if (!jsonValue.is<const char*>()) {
        return false;
    }

    const char* stringValue = jsonValue.as<const char*>();
    if (stringValue == nullptr) {
        return false;
    }

    value.assign(stringValue);
    return true;
}

bool readInt(JsonObjectConst object, const char* key, int& value) {
    const JsonVariantConst jsonValue = object[key];
    if (!jsonValue.is<int>()) {
        return false;
    }

    value = jsonValue.as<int>();
    return true;
}

bool readUint8(JsonObjectConst object, const char* key, uint8_t& value) {
    const JsonVariantConst jsonValue = object[key];
    if (!jsonValue.is<uint8_t>()) {
        return false;
    }

    value = jsonValue.as<uint8_t>();
    return true;
}

bool readBool(JsonObjectConst object, const char* key, bool& value) {
    const JsonVariantConst jsonValue = object[key];
    if (!jsonValue.is<bool>()) {
        return false;
    }

    value = jsonValue.as<bool>();
    return true;
}

bool readObject(JsonObjectConst object, const char* key,
                JsonObjectConst& value) {
    const JsonVariantConst jsonValue = object[key];
    if (!jsonValue.is<JsonObjectConst>()) {
        return false;
    }

    value = jsonValue.as<JsonObjectConst>();
    return true;
}

bool readArray(JsonObjectConst object, const char* key, JsonArrayConst& value) {
    const JsonVariantConst jsonValue = object[key];
    if (!jsonValue.is<JsonArrayConst>()) {
        return false;
    }

    value = jsonValue.as<JsonArrayConst>();
    return true;
}

}  // namespace

bool validOtaPort(int port) {
    return port >= 1 && port <= 65535;
}

bool validWifiChannel(uint8_t channel) {
    return channel >= 1 && channel <= kMaxWifiChannel;
}

bool validWifiPower(uint8_t power) {
    return power <= kMaxWifiPower;
}

bool validHostname(const std::string& hostname) {
    for (char character : hostname) {
        if ((character >= '0' && character <= '9') ||
            (character >= 'A' && character <= 'Z') ||
            (character >= 'a' && character <= 'z') || character == '-' ||
            character == '.') {
            continue;
        }
        return false;
    }
    return true;
}

// Escape a raw string for interpolation into the hand-built JSON produced by
// the toRepresentation() helpers: backslash, double quote, and control
// characters would otherwise terminate or corrupt the JSON document.
std::string jsonEscape(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (char rawCharacter : value) {
        const unsigned char character =
            static_cast<unsigned char>(rawCharacter);
        switch (character) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                if (character < 0x20) {
                    char unicodeBuffer[8];
                    snprintf(unicodeBuffer, sizeof(unicodeBuffer), "\\u%04X",
                             static_cast<unsigned int>(character));
                    escaped += unicodeBuffer;
                } else {
                    escaped += rawCharacter;
                }
                break;
        }
    }
    return escaped;
}

bool parseSnapshot(const std::string& serialized, config_snapshot& snapshot) {
    JsonDocument document;
    const DeserializationError error =
        deserializeJson(document, serialized.c_str());
    if (error || !document.is<JsonObjectConst>()) {
        return false;
    }

    const JsonObjectConst root = document.as<JsonObjectConst>();
    if (!readUint8(root, "version", snapshot.version) ||
        snapshot.version != config_snapshot::current_version) {
        return false;
    }

    JsonObjectConst device;
    if (!readObject(root, "device", device) ||
        !readString(device, "ota_login", snapshot.device.ota_login) ||
        !readString(device, "ota_password", snapshot.device.ota_password) ||
        !readInt(device, "ota_port", snapshot.device.ota_port) ||
        !validOtaPort(snapshot.device.ota_port)) {
        return false;
    }
    if (snapshot.device.ota_login.empty() !=
        snapshot.device.ota_password.empty()) {
        return false;
    }

    JsonObjectConst deviceData;
    if (!readObject(root, "device_data", deviceData) ||
        !readString(deviceData, "deviceJson",
                    snapshot.device_data.deviceJson)) {
        return false;
    }

    if (!readObject(root, "mdns", deviceData) ||
        !readString(deviceData, "hostname", snapshot.mdns.hostname) ||
        !validHostname(snapshot.mdns.hostname)) {
        return false;
    }

    if (!readObject(root, "wifi_tx_power", deviceData) ||
        !readUint8(deviceData, "power", snapshot.wifi_tx_power.power) ||
        !validWifiPower(snapshot.wifi_tx_power.power)) {
        return false;
    }

    JsonObjectConst apNetwork;
    if (!readObject(root, "ap_network", apNetwork) ||
        !readString(apNetwork, "ssid", snapshot.ap_network.ssid) ||
        !readString(apNetwork, "password", snapshot.ap_network.password) ||
        !readUint8(apNetwork, "channel", snapshot.ap_network.channel) ||
        !readBool(apNetwork, "adhoc", snapshot.ap_network.adhoc) ||
        !validWifiChannel(snapshot.ap_network.channel)) {
        return false;
    }

    uint8_t networkCount = 0;
    JsonArrayConst networks;
    if (!readUint8(root, "network_count", networkCount) ||
        networkCount > kMaxSavedNetworks ||
        !readArray(root, "networks", networks) ||
        networks.size() != networkCount) {
        return false;
    }

    snapshot.networks.clear();
    snapshot.networks.reserve(networkCount);
    for (JsonVariantConst value : networks) {
        if (!value.is<JsonObjectConst>()) {
            return false;
        }

        const JsonObjectConst network = value.as<JsonObjectConst>();
        Project_Config::WiFiConfig_t savedNetwork("", "", "", 0, 0, false);
        if (!readString(network, "name", savedNetwork.name) ||
            !readString(network, "ssid", savedNetwork.ssid) ||
            !readString(network, "password", savedNetwork.password) ||
            !readUint8(network, "channel", savedNetwork.channel) ||
            !readUint8(network, "power", savedNetwork.power) ||
            !readBool(network, "adhoc", savedNetwork.adhoc) ||
            !validWifiChannel(savedNetwork.channel) ||
            !validWifiPower(savedNetwork.power)) {
            return false;
        }
        snapshot.networks.push_back(std::move(savedNetwork));
    }

    return true;
}

bool serializeSnapshot(const config_snapshot& snapshot,
                       std::string& serialized) {
    if (snapshot.version != config_snapshot::current_version ||
        snapshot.networks.size() > kMaxSavedNetworks ||
        !validOtaPort(snapshot.device.ota_port) ||
        // Never persist a partial management credential pair; parseSnapshot
        // rejects one, so serialization must reject it symmetrically.
        snapshot.device.ota_login.empty() !=
            snapshot.device.ota_password.empty() ||
        !validHostname(snapshot.mdns.hostname) ||
        !validWifiPower(snapshot.wifi_tx_power.power) ||
        !validWifiChannel(snapshot.ap_network.channel)) {
        return false;
    }

    for (const auto& network : snapshot.networks) {
        if (!validWifiChannel(network.channel) ||
            !validWifiPower(network.power)) {
            return false;
        }
    }

    JsonDocument document;
    JsonObject root = document.to<JsonObject>();
    root["version"] = snapshot.version;

    JsonObject device = root["device"].to<JsonObject>();
    device["ota_login"] = snapshot.device.ota_login.c_str();
    device["ota_password"] = snapshot.device.ota_password.c_str();
    device["ota_port"] = snapshot.device.ota_port;

    JsonObject deviceData = root["device_data"].to<JsonObject>();
    deviceData["deviceJson"] = snapshot.device_data.deviceJson.c_str();

    JsonObject mdns = root["mdns"].to<JsonObject>();
    mdns["hostname"] = snapshot.mdns.hostname.c_str();

    JsonObject wifiTxPower = root["wifi_tx_power"].to<JsonObject>();
    wifiTxPower["power"] = snapshot.wifi_tx_power.power;

    JsonObject apNetwork = root["ap_network"].to<JsonObject>();
    apNetwork["ssid"] = snapshot.ap_network.ssid.c_str();
    apNetwork["password"] = snapshot.ap_network.password.c_str();
    apNetwork["channel"] = snapshot.ap_network.channel;
    apNetwork["adhoc"] = snapshot.ap_network.adhoc;

    root["network_count"] = snapshot.networks.size();
    JsonArray networks = root["networks"].to<JsonArray>();
    for (const auto& network : snapshot.networks) {
        JsonObject savedNetwork = networks.add<JsonObject>();
        savedNetwork["name"] = network.name.c_str();
        savedNetwork["ssid"] = network.ssid.c_str();
        savedNetwork["password"] = network.password.c_str();
        savedNetwork["channel"] = network.channel;
        savedNetwork["power"] = network.power;
        savedNetwork["adhoc"] = network.adhoc;
    }

    if (document.overflowed()) {
        return false;
    }

    serialized.clear();
    serializeJson(document, serialized);
    return !serialized.empty() && serialized.size() <= kMaxSnapshotJsonBytes;
}
