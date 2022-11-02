#ifndef STATEMANAGER_HPP
#define STATEMANAGER_HPP
#include <Arduino.h>

/*
 * StateManager
 * All Project States are managed here
 */
namespace ProgramStates
{
    struct DeviceStates
    {
        enum State_e
        {
            Starting,
            Started,
            Stopping,
            Stopped,
            Error
        };

        enum WiFiState_e
        {
            WiFiState_None,
            WiFiState_Connecting,
            WiFiState_Connected,
            WiFiState_Disconnected,
            WiFiState_Disconnecting,
            WiFiState_ADHOC,
            WiFiState_Error
        };

        enum WebServerState_e
        {
            WebServerState_None,
            WebServerState_Starting,
            WebServerState_Started,
            WebServerState_Stopping,
            WebServerState_Stopped,
            WebServerState_Error
        };

        enum MDNSState_e
        {
            MDNSState_None,
            MDNSState_Starting,
            MDNSState_Started,
            MDNSState_Stopping,
            MDNSState_Stopped,
            MDNSState_Error
        };
    };
};

/*
 * EventManager
 * All Project Events are managed here
 */
template <class T>
class StateManager
{
public:
    StateManager() {}

    virtual ~StateManager() {}

    void setState(T state)
    {
        _current_state = state;
    }

    /*
     * Get States
     * Returns the current state of the device
     */
    T getCurrentState()
    {
        return _current_state;
    }

private:
    T _current_state;
};

typedef ProgramStates::DeviceStates::State_e State_e;
typedef ProgramStates::DeviceStates::WiFiState_e WiFiState_e;
typedef ProgramStates::DeviceStates::WebServerState_e WebServerState_e;
typedef ProgramStates::DeviceStates::MDNSState_e MDNSState_e;

extern StateManager<State_e> stateManager;
extern StateManager<WiFiState_e> wifiStateManager;
extern StateManager<WebServerState_e> webServerStateManager;
extern StateManager<MDNSState_e> mdnsStateManager;

#endif // STATEMANAGER_HPP