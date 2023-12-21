#pragma once

//! Required header files
#include <api/rest_api_handler.hpp>        //! (*)
#include <data/config/config_handler.hpp>  //! (*)
#include <utilities/states.hpp>            //! (*)

#include <network/mdns/mdns_manager.hpp>

#if ENABLE_ETHERNET
#    include <network/wifihandler/ethernet/ethernet_handler.hpp>  //! (*)
#else
#    include <network/wifihandler/wifi_handler.hpp>  //! (*)
#endif

class EasyNetworkManager {
   public:
    EasyNetworkManager(const std::string& config_name,
                       const std::string& hostname, const std::string& ssid,
                       const std::string& password, int channel,
                       const std::string& service_name,
                       const std::string& service_instance_name,
                       const std::string& service_protocol,
                       const std::string& service_description,
                       const std::string& service_port,
                       bool enable_mdns = false, bool enable_adhoc = false);
    virtual ~EasyNetworkManager();

    void begin();

    /**
     * @brief
     * @note The Project Config Manager is used to store and retrieve the
     * configuration
     * @param config_name The name of the project (used to create the config
     * file name)
     * @param hostname The hostname for your device on the network(used for
     * mDNS, OTA, etc.)
     */
    std::shared_ptr<ConfigHandler> configHandler;

    /**
     * @brief The WiFi Handler is used to manage the WiFi connection
     * @note The WiFi Handler constructor takes four parameters:
     * @param configHandler A pointer to the config manager
     * @param wifi_ssid The SSID of the WiFi network to connect to
     * @param wifi_password The password of the WiFi network to connect to
     * @param wifi_channel The channel of the WiFi network to connect to
     */
    std::shared_ptr<WiFiHandler> wifiHandler;

    /**
     * @brief The mDNS Manager is used to create a mDNS service for the device
     * @note Service name and service protocol have to be lowercase and begin
     * with an underscore
     * @param configHandler A reference to the config manager
     * @param service_name The name of the service
     * @param service_instance_name The instance name of the service
     * @param service_protocol The protocol of the service
     * @param service_description The description of the service
     * @param service_port The port of the service
     */
    std::shared_ptr<MDNSHandler> mdnsHandler = nullptr;
};