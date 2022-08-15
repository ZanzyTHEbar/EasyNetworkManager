# EasyNetworkManager Library

This is an in-progress library for easy network management. It is not yet complete, but it is a good start (and functional).

This library implements the following classes:

- APIServer - A server that can be used to manage asynchronous REST API methods.
- WiFiHandler - A class that can be used to manage WiFi connections.
- OTA - A basic OTA handler.
- MDNSHandler - A class that can be used to manage mDNS services.
- Config - A class that can be used to manage configuration files - uses my [EasyPreferencesLibrary project](https://github.com/ZanzyTHEbar/EasyPreferencesLibrary).
- StateManager - A class that can be used to manage the state of your project (very easy to extend).
- Utilities - A folder that has various template files that can be used to manage various utilities.

## Installation

#### Platformio (recommended)

In your `platformio.ini` file add the following:

```ini
lib_deps = 
    https://github.com/ZanzyTHEbar/EasyNetworkManager.git
```

#### Arduino IDE

To install this library in your Arduino IDE, you must add all dependencies (sorry) and then download this repository as a zip file and it as any other library :smile:.

## Dependencies (required)

- [EasyPreferencesLibrary project](https://github.com/ZanzyTHEbar/EasyPreferencesLibrary)

All other dependencies _should_ be installed automatically. If not, please make a new issue and I will fix it.

#### Dependencies used in this project

- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer.git)
- [AsyncTCP](https://github.com/me-no-dev/AsyncTCP.git)
- [EasyPreferencesLibrary project](https://github.com/ZanzyTHEbar/EasyPreferencesLibrary)

## Usage

For basic usage please see the [examples](/NetworkManager/examples) folder.

To extend any of the enums please use the `data/utilities/enuminheritance.hpp` file.

To extend any of the config sections, simply create a namespace with the same name as the config struct is in and add your own `struct` to it. For example, to extend the `DeviceConfig_t` `struct`, you would do the following:

```cpp
namespace Project_Config {

    struct NewConfig_t {
        DeviceConfig_t device;
        String newConfig;
        int newint;
        bool newbool;
    };
}

Project_Config::NewConfig_t newConfig; // this creates a new object of your config struct.
```

> **Note**: This library is still in development and is not yet complete, if there are any bugs please report them in the issues section.

## Modification

This library is intended to be modified and extended. If you find any bugs, please make a new issue and I will fix it.

If you have any questions, please ask in the [discussions](https://github.com/ZanzyTHEbar/EasyNetworkManager/discussions).

## License

This library is licensed under the MIT License.
