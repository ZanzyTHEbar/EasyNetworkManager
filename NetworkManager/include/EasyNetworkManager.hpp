#ifndef EASY_NETWORK_MANAGER_HPP
#define EASY_NETWORK_MANAGER_HPP

//! Required header files
#include <api/webserverHandler.hpp>             //! (*)
#include <data/statemanager/StateManager.hpp>   //! (*)
#include <data/config/project_config.hpp>       //! (*)

#if ENABLE_ETHERNET
#include <network/wifihandler/ethernet/ethernethandler.hpp>  //! (*)
#else
#include <network/wifihandler/wifihandler.hpp>  //! (*)
#endif
#endif // EASY_NETWORK_MANAGER_HPP