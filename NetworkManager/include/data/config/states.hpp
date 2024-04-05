#pragma once

#include <algorithm>
#include <type_traits>
#include <variant>

/*                                     \
 * StateManager                        \
 * All Project States are managed here \
 */
namespace ProgramStates {
enum ProgramState_e { Starting, Started, Stopping, Stopped, Error };
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

enum ProjectConfigEventIDs_e {
    ProjectConfigEventID_ConfigHandler,
    ProjectConfigEventID_MDNSHandler,
    ProjectConfigEventID_WifiHandler,
};

};  // namespace ProgramStates

using State_e = ProgramStates::ProgramState_e;
using Event_e = ProgramStates::Event_e;
using WiFiState_e = ProgramStates::WiFiState_e;
using WebServerState_e = ProgramStates::WebServerState_e;
using MDNSState_e = ProgramStates::MDNSState_e;
using ProjectConfigEventIDs_e = ProgramStates::ProjectConfigEventIDs_e;

using StateVariant =
    std::variant<State_e, Event_e, WiFiState_e, WebServerState_e, MDNSState_e>;

// Define a generic function that applies a switch-case logic to an enum
// variant.
template <typename EnumType, typename VariantType, typename Func>
void updateStateWrapper(const VariantType& _variant, Func&& _switchCaseFunc) {
    std::visit(
        [&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, EnumType>) {
                _switchCaseFunc(arg);
            }
        },
        _variant);
}