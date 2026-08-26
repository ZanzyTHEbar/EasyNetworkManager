#include <unity.h>

#include <string>

#include <data/config/config_snapshot_codec.hpp>

void setUp() {}
void tearDown() {}

namespace {

config_snapshot makeValidSnapshot() {
    config_snapshot snapshot;
    snapshot.device.ota_login = "admin";
    snapshot.device.ota_password = "secret";
    snapshot.device.ota_port = 3232;
    snapshot.device_data.deviceJson = "{\"id\":42}";
    snapshot.mdns.hostname = "esp32custom";
    snapshot.wifi_tx_power.power = 52;
    snapshot.ap_network.ssid = "ESP32-AP";
    snapshot.ap_network.password = "ap-pass";
    snapshot.ap_network.channel = 6;
    snapshot.ap_network.adhoc = false;
    snapshot.networks.emplace_back("Home", "home-ssid", "home-pass", 1, 78,
                                   false);
    snapshot.networks.emplace_back("Office", "office-ssid", "office-pass", 11,
                                   40, false);
    return snapshot;
}

// Minimal valid snapshot JSON; individual tests mutate one field at a time.
const char* kBaseSnapshot =
    "{\"version\":1,"
    "\"device\":{\"ota_login\":\"admin\",\"ota_password\":\"secret\","
    "\"ota_port\":3232},"
    "\"device_data\":{\"deviceJson\":\"{}\"},"
    "\"mdns\":{\"hostname\":\"esp32custom\"},"
    "\"wifi_tx_power\":{\"power\":52},"
    "\"ap_network\":{\"ssid\":\"AP\",\"password\":\"pw\",\"channel\":6,"
    "\"adhoc\":false},"
    "\"network_count\":0,\"networks\":[]}";

std::string withReplacement(const char* from, const std::string& needle,
                            const std::string& replacement) {
    std::string json(from);
    const size_t position = json.find(needle);
    TEST_ASSERT_NOT_EQUAL(std::string::npos, position);
    return json.replace(position, needle.size(), replacement);
}

}  // namespace

void test_round_trip_full_snapshot() {
    const config_snapshot snapshot = makeValidSnapshot();
    std::string serialized;
    TEST_ASSERT_TRUE(serializeSnapshot(snapshot, serialized));

    config_snapshot parsed;
    TEST_ASSERT_TRUE(parseSnapshot(serialized, parsed));
    TEST_ASSERT_EQUAL_UINT8(snapshot.version, parsed.version);
    TEST_ASSERT_EQUAL_STRING(snapshot.device.ota_login.c_str(),
                             parsed.device.ota_login.c_str());
    TEST_ASSERT_EQUAL_STRING(snapshot.device.ota_password.c_str(),
                             parsed.device.ota_password.c_str());
    TEST_ASSERT_EQUAL_INT(snapshot.device.ota_port, parsed.device.ota_port);
    TEST_ASSERT_EQUAL_STRING(snapshot.device_data.deviceJson.c_str(),
                             parsed.device_data.deviceJson.c_str());
    TEST_ASSERT_EQUAL_STRING(snapshot.mdns.hostname.c_str(),
                             parsed.mdns.hostname.c_str());
    TEST_ASSERT_EQUAL_UINT8(snapshot.wifi_tx_power.power,
                            parsed.wifi_tx_power.power);
    TEST_ASSERT_EQUAL_STRING(snapshot.ap_network.ssid.c_str(),
                             parsed.ap_network.ssid.c_str());
    TEST_ASSERT_EQUAL_STRING(snapshot.ap_network.password.c_str(),
                             parsed.ap_network.password.c_str());
    TEST_ASSERT_EQUAL_UINT8(snapshot.ap_network.channel,
                            parsed.ap_network.channel);
    TEST_ASSERT_TRUE(snapshot.ap_network.adhoc == parsed.ap_network.adhoc);

    TEST_ASSERT_EQUAL_UINT32(2, parsed.networks.size());
    TEST_ASSERT_EQUAL_STRING("Home", parsed.networks[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("home-ssid", parsed.networks[0].ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("home-pass", parsed.networks[0].password.c_str());
    TEST_ASSERT_EQUAL_UINT8(1, parsed.networks[0].channel);
    TEST_ASSERT_EQUAL_UINT8(78, parsed.networks[0].power);
    TEST_ASSERT_FALSE(parsed.networks[0].adhoc);
    TEST_ASSERT_EQUAL_STRING("Office", parsed.networks[1].name.c_str());
    TEST_ASSERT_EQUAL_STRING("office-ssid", parsed.networks[1].ssid.c_str());
    TEST_ASSERT_EQUAL_UINT8(11, parsed.networks[1].channel);
}

void test_version_mismatch_rejected_on_parse() {
    std::string serialized;
    TEST_ASSERT_TRUE(serializeSnapshot(makeValidSnapshot(), serialized));
    const size_t position = serialized.find("\"version\":1");
    TEST_ASSERT_NOT_EQUAL(std::string::npos, position);
    serialized.replace(position, 11, "\"version\":2");

    config_snapshot parsed;
    TEST_ASSERT_FALSE(parseSnapshot(serialized, parsed));
}

void test_partial_credentials_rejected_on_parse() {
    const std::string json =
        withReplacement(kBaseSnapshot, "\"ota_password\":\"secret\",", "");
    config_snapshot parsed;
    TEST_ASSERT_FALSE(parseSnapshot(json, parsed));
}

void test_partial_credentials_rejected_on_serialize() {
    config_snapshot snapshot = makeValidSnapshot();
    snapshot.device.ota_password.clear();
    std::string serialized;
    TEST_ASSERT_FALSE(serializeSnapshot(snapshot, serialized));
}

void test_network_count_above_limit_rejected_on_parse() {
    const std::string networkEntry =
        "{\"name\":\"n\",\"ssid\":\"s\",\"password\":\"p\",\"channel\":6,"
        "\"power\":52,\"adhoc\":false}";
    std::string networks;
    for (int i = 0; i < 4; i++) {
        if (i > 0)
            networks += ",";
        networks += networkEntry;
    }
    const std::string json =
        withReplacement(kBaseSnapshot, "\"network_count\":0,\"networks\":[]",
                        "\"network_count\":4,\"networks\":[" + networks + "]");
    config_snapshot parsed;
    TEST_ASSERT_FALSE(parseSnapshot(json, parsed));
}

void test_network_array_size_mismatch_rejected_on_parse() {
    const std::string json = withReplacement(
        kBaseSnapshot, "\"network_count\":0,\"networks\":[]",
        "\"network_count\":1,\"networks\":[{\"name\":\"n\",\"ssid\":\"s\","
        "\"password\":\"p\",\"channel\":6,\"power\":52,\"adhoc\":false},{"
        "\"name\":\"n2\",\"ssid\":\"s2\",\"password\":\"p2\",\"channel\":6,"
        "\"power\":52,\"adhoc\":false}]");
    config_snapshot parsed;
    TEST_ASSERT_FALSE(parseSnapshot(json, parsed));
}

void test_ap_channel_zero_and_fifteen_rejected_on_parse() {
    for (const char* channel : {"0", "15"}) {
        const std::string json =
            withReplacement(kBaseSnapshot, "\"channel\":6,",
                            std::string("\"channel\":") + channel + ",");
        config_snapshot parsed;
        TEST_ASSERT_FALSE(parseSnapshot(json, parsed));
    }
}

void test_ap_channel_out_of_range_rejected_on_serialize() {
    config_snapshot snapshot = makeValidSnapshot();
    snapshot.ap_network.channel = 15;
    std::string serialized;
    TEST_ASSERT_FALSE(serializeSnapshot(snapshot, serialized));
}

void test_wifi_power_79_rejected_on_parse() {
    const std::string json =
        withReplacement(kBaseSnapshot, "\"power\":52", "\"power\":79");
    config_snapshot parsed;
    TEST_ASSERT_FALSE(parseSnapshot(json, parsed));
}

void test_wifi_power_79_rejected_on_serialize() {
    config_snapshot snapshot = makeValidSnapshot();
    snapshot.wifi_tx_power.power = 79;
    std::string serialized;
    TEST_ASSERT_FALSE(serializeSnapshot(snapshot, serialized));
}

void test_ota_port_zero_and_70000_rejected_on_parse() {
    for (const char* port : {"0", "70000"}) {
        const std::string json =
            withReplacement(kBaseSnapshot, "\"ota_port\":3232",
                            std::string("\"ota_port\":") + port);
        config_snapshot parsed;
        TEST_ASSERT_FALSE(parseSnapshot(json, parsed));
    }
}

void test_ota_port_zero_and_70000_rejected_on_serialize() {
    config_snapshot zeroPort = makeValidSnapshot();
    zeroPort.device.ota_port = 0;
    std::string serialized;
    TEST_ASSERT_FALSE(serializeSnapshot(zeroPort, serialized));

    config_snapshot highPort = makeValidSnapshot();
    highPort.device.ota_port = 70000;
    TEST_ASSERT_FALSE(serializeSnapshot(highPort, serialized));
}

void test_invalid_hostname_character_rejected_on_parse() {
    const std::string json =
        withReplacement(kBaseSnapshot, "\"hostname\":\"esp32custom\"",
                        "\"hostname\":\"esp32_custom\"");
    config_snapshot parsed;
    TEST_ASSERT_FALSE(parseSnapshot(json, parsed));
}

void test_invalid_hostname_character_rejected_on_serialize() {
    config_snapshot snapshot = makeValidSnapshot();
    snapshot.mdns.hostname = "my_host";
    std::string serialized;
    TEST_ASSERT_FALSE(serializeSnapshot(snapshot, serialized));
}

void test_json_escape_escapes_quote_backslash_and_newline() {
    // Input: a"b\c<newline>d
    const std::string input = "a\"b\\c\nd";
    TEST_ASSERT_EQUAL_STRING("a\\\"b\\\\c\\nd", jsonEscape(input).c_str());
}

void test_oversized_payload_rejected_by_serialize() {
    config_snapshot snapshot = makeValidSnapshot();
    snapshot.device_data.deviceJson = std::string(4000, 'x');
    std::string serialized;
    TEST_ASSERT_FALSE(serializeSnapshot(snapshot, serialized));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_round_trip_full_snapshot);
    RUN_TEST(test_version_mismatch_rejected_on_parse);
    RUN_TEST(test_partial_credentials_rejected_on_parse);
    RUN_TEST(test_partial_credentials_rejected_on_serialize);
    RUN_TEST(test_network_count_above_limit_rejected_on_parse);
    RUN_TEST(test_network_array_size_mismatch_rejected_on_parse);
    RUN_TEST(test_ap_channel_zero_and_fifteen_rejected_on_parse);
    RUN_TEST(test_ap_channel_out_of_range_rejected_on_serialize);
    RUN_TEST(test_wifi_power_79_rejected_on_parse);
    RUN_TEST(test_wifi_power_79_rejected_on_serialize);
    RUN_TEST(test_ota_port_zero_and_70000_rejected_on_parse);
    RUN_TEST(test_ota_port_zero_and_70000_rejected_on_serialize);
    RUN_TEST(test_invalid_hostname_character_rejected_on_parse);
    RUN_TEST(test_invalid_hostname_character_rejected_on_serialize);
    RUN_TEST(test_json_escape_escapes_quote_backslash_and_newline);
    RUN_TEST(test_oversized_payload_rejected_by_serialize);
    return UNITY_END();
}
