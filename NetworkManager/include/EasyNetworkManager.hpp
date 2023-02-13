#ifndef EASY_NETWORK_MANAGER_HPP
#define EASY_NETWORK_MANAGER_HPP

//! Required header files
#include <api/webserverHandler.hpp>             //! (*)
#include <data/StateManager/StateManager.hpp>   //! (*)
#include <data/config/project_config.hpp>       //! (*)

#if ENABLE_ETHERNET
#include <network/wifiHandler/ethernet/ethernetHandler.hpp>  //! (*)
#else
#include <network/wifihandler/wifiHandler.hpp>  //! (*)
#endif
#endif // EASY_NETWORK_MANAGER_HPP