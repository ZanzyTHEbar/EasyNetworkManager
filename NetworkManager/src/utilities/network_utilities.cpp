#include <utilities/network_utilities.hpp>

/**
 * @brief Function to setup the WiFi scan and set the WiFi mode to station.
 *
 * @brief Call this function in the setup() function
 */
void Network_Utilities::setupWifiScan() {
    // Set WiFi to station mode and disconnect from an AP if it was previously
    // connected
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();  // Disconnect from the access point if connected before
    delay(100);

    Serial.println("Setup done");
}

bool Network_Utilities::loopWifiScan() {
    // WiFi.scanNetworks will return the number of networks found
    log_i("[INFO]: Beginning WiFi Scanner");
    int networks = WiFi.scanNetworks();
    log_i("[INFO]: scan done");

    log_i("%d networks found", networks);
    for (int i = networks; i--;) {
        // Print SSID and RSSI for each network found
        //! Add method here to interface with the API and forward the scanned
        //! networks to the API
        log_i("%d: %s (%d) %s\n", i - 1, WiFi.SSID(i), WiFi.RSSI(i),
              (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
        my_delay(0.02L);  // delay 20ms
    }

    // Wait a bit before scanning again
    delay(5000);
    return (networks > 0);
}

// Take measurements of the Wi-Fi strength and return the average result.
int Network_Utilities::getStrength(int points)  // TODO: add to JSON doc
{
    int32_t rssi = 0, averageRSSI = 0;

    for (int i = 0; i < points; i++) {
        rssi += WiFi.RSSI();
        delay(20);
    }

    averageRSSI = rssi / points;
    return averageRSSI;
}

void Network_Utilities::my_delay(volatile long delay_time) {
    delay_time = delay_time * 1e6L;
    for (volatile long count = delay_time; count > 0L; count--)
        ;
}

// a function to generate the device ID
std::string Network_Utilities::generateDeviceID() {
    uint32_t chipId = 0;
    for (int i = 0; i < 17; i = i + 8) {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }

    log_i("ESP Chip model = %s Rev %d\n", ESP.getChipModel(),
          ESP.getChipRevision());
    log_i("This chip has %d cores\n", ESP.getChipCores());
    log_i("Chip ID: %d", chipId);
    std::string deviceID = (const char*)chipId;
    return deviceID;
}

/**
 * @brief Function to map the WiFi status to the WiFiState_e enum
 *
 * @brief Call this function in the loop() function
 */
static void Network_Utilities::checkWiFiState() {
    if (wifiStateManager.getCurrentState() == WiFiState_e::WiFiState_ADHOC) {
        return;
    }

    switch (WiFi.status()) {
        case wl_status_t::WL_IDLE_STATUS:
            wifiStateManager.setState(WiFiState_e::WiFiState_Idle);
        case wl_status_t::WL_NO_SSID_AVAIL:
            wifiStateManager.setState(WiFiState_e::WiFiState_Error);
        case wl_status_t::WL_SCAN_COMPLETED:
            wifiStateManager.setState(WiFiState_e::WiFiState_None);
        case wl_status_t::WL_CONNECTED:
            wifiStateManager.setState(WiFiState_e::WiFiState_Connected);
        case wl_status_t::WL_CONNECT_FAILED:
            wifiStateManager.setState(WiFiState_e::WiFiState_Error);
        case wl_status_t::WL_CONNECTION_LOST:
            wifiStateManager.setState(WiFiState_e::WiFiState_Disconnected);
        case wl_status_t::WL_DISCONNECTED:
            wifiStateManager.setState(WiFiState_e::WiFiState_Disconnected);
        default:
            wifiStateManager.setState(WiFiState_e::WiFiState_None);
    }
}

std::string Network_Utilities::shaEncoder(const std::string& data) {
    const char* data_c = data.c_str();
    int size = 64;
    uint8_t hash[size];
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA512;

    const size_t len = strlen(data_c);
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const unsigned char*)data_c, len);
    mbedtls_md_finish(&ctx, hash);
    mbedtls_md_free(&ctx);

    std::string hash_string = "";
    for (uint16_t i = 0; i < size; i++) {
        std::string hex = String(hash[i], HEX).c_str();
        if (hex.length() < 2) {
            hex = "0" + hex;
        }
        hash_string += hex;
    }
    return hash_string;
}