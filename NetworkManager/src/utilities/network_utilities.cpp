#include <utilities/network_utilities.hpp>

/**
 * @brief Function to setup the WiFi scan and set the WiFi mode to station.
 * @note Call this function in the setup() function
 */
void Network_Utilities::setupWifiScan() {
    // Set WiFi to station mode and disconnect from an AP if it was previously
    // connected
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();  // Disconnect from the access point if connected before
    delay(100);

    Serial.println("Setup done");
}

/**
 * @brief Function to scan for WiFi networks and print the results to the serial
 * @note Call this function in the loop() function
 * @return true
 * @return false
 */
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

/**
 * @brief Function to get the strength of the WiFi signal
 *
 * @param points int Number of points to average
 * @return int
 */
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

/**
 * @brief Function to delay the program execution
 *
 * @param delay_time volatile long
 */
void Network_Utilities::my_delay(volatile long delay_time) {
    delay_time = delay_time * 1e6L;
    for (volatile long count = delay_time; count > 0L; count--)
        ;
}

/**
 * @brief Function to generate a device ID from the ESP32 chip ID
 *
 * @return std::string
 */
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
