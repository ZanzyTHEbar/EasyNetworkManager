#pragma once
#include <variant>
#include "state_manager.hpp"

/*                                     \
 * StateManager                        \
 * All Project States are managed here \
 */
namespace ProgramStates {
enum State_e { Starting, Started, Stopping, Stopped, Error };
enum Event_e {
    configLoaded,
    configSaved,
    deviceConfigUpdated,
    mdnsConfigUpdated,
    networksConfigUpdated,
    apConfigUpdated,
    wifiTxPowerUpdated,
    deviceDataJsonUpdated,
};

enum WiFiState_e {
    WiFiState_None,
    WiFiState_Idle,
    WiFiState_Scanning,
    WiFiState_Scanning_Done,
    WiFiState_Connecting,
    WiFiState_Connected,
    WiFiState_Disconnected,
    WiFiState_Disconnecting,
    WiFiState_ADHOC,
    WiFiState_Error
};

enum WebServerState_e {
    WebServerState_None,
    WebServerState_Starting,
    WebServerState_Started,
    WebServerState_Stopping,
    WebServerState_Stopped,
    WebServerState_Error
};

enum MDNSState_e {
    MDNSState_None,
    MDNSState_Starting,
    MDNSState_Started,
    MDNSState_Stopping,
    MDNSState_Stopped,
    MDNSState_Error
};
};  // namespace ProgramStates

typedef ProgramStates::State_e State_e;
typedef ProgramStates::Event_e Event_e;
typedef ProgramStates::WiFiState_e WiFiState_e;
typedef ProgramStates::WebServerState_e WebServerState_e;
typedef ProgramStates::MDNSState_e MDNSState_e;

//* Define a broad variant
using StateVariant =
    std::variant<State_e, Event_e, WiFiState_e, WebServerState_e, MDNSState_e>;
// extern StateManager<StateVariant> stateManager;