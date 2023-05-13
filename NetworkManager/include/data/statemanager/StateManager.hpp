#ifndef STATEMANAGER_HPP
#define STATEMANAGER_HPP

/*
 * StateManager
 * All Project States are managed here
 */
namespace ProgramStates {
struct DeviceStates {
    enum State_e { Starting, Started, Stopping, Stopped, Error };

    enum Event_e {
        configLoaded,
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
};
};  // namespace ProgramStates

/*
 * EventManager
 * All Project Events are managed here
 */
template <class T>
class StateManager {
   public:
    StateManager() {}

    virtual ~StateManager() {
        this->_current_state = static_cast<T>(0);
    }

    /*
     * @brief Sets the  state of the stateManager
     * @param T state - the state to be set
     */
    void setState(T state) {
        _current_state = state;
    }

    /*
     * Get States
     * Returns the current state of the device
     */
    T getCurrentState() {
        return _current_state;
    }

   protected:
    T _current_state;
};

typedef ProgramStates::DeviceStates::State_e State_e;
typedef ProgramStates::DeviceStates::Event_e Event_e;
typedef ProgramStates::DeviceStates::WiFiState_e WiFiState_e;
typedef ProgramStates::DeviceStates::WebServerState_e WebServerState_e;
typedef ProgramStates::DeviceStates::MDNSState_e MDNSState_e;

extern StateManager<Event_e> eventManager;
extern StateManager<State_e> stateManager;
extern StateManager<WiFiState_e> wifiStateManager;
extern StateManager<WebServerState_e> webServerStateManager;
extern StateManager<MDNSState_e> mdnsStateManager;

#endif  // STATEMANAGER_HPP