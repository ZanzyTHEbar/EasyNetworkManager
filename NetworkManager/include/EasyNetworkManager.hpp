#ifndef EASY_NETWORK_MANAGER_HPP
#define EASY_NETWORK_MANAGER_HPP

//! Required header files
#include <api/rest_api_handler.hpp>            //! (*)
#include <data/config/config_handler.hpp>      //! (*)
#include <data/statemanager/state_manager.hpp>  //! (*)

#if ENABLE_ETHERNET
#    include <network/wifihandler/ethernet/ethernet_handler.hpp>  //! (*)
#else
#    include <network/wifihandler/wifi_handler.hpp>  //! (*)
#endif
#endif  // EASY_NETWORK_MANAGER_HPP