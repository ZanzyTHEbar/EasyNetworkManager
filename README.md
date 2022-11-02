# EasyNetworkManager Library

This is an in-progress library for easy network management.

This library implements the following classes:

- APIServer - A server that can be used to manage asynchronous REST API methods.
    - has a `handleJSON` method for handling `POST` and `GET` requests. Can send and receive JSON. 
	> **Note**: `POST` requests for `JSON` are still in development.
- WiFiHandler - A class that can be used to manage WiFi connections.
- OTA - A basic OTA handler.
- MDNSHandler - A class that can be used to manage mDNS services.
- Config - A class that can be used to manage configuration files - uses my [EasyPreferencesLibrary project](https://github.com/ZanzyTHEbar/EasyPreferencesLibrary).
- StateManager - A class that can be used to manage the state of your project (very easy to extend).
- Utilities - A folder that has various template files that can be used to manage various utilities.

## Installation

#### Platformio (recommended)

You can install via the `Platformio Registry` by navigating to the `Libraries` section of `Platformio`. 
The library is called `EasyNetworkManager` by `ZanzyTHEbar`.

You can also install via a github link:

In your `platformio.ini` file add the following:

```ini
lib_deps = 
    https://github.com/ZanzyTHEbar/EasyNetworkManager.git
```

#### Arduino IDE

To install this library in your Arduino IDE, you must add all dependencies (sorry) and then download this repository as a zip file and it as any other library :smile:.

## Dependencies

All dependencies _should_ be installed automatically. If not, please make a new issue and I will fix it.

#### Dependencies used in this project

- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer.git)
- [AsyncTCP](https://github.com/me-no-dev/AsyncTCP.git)

## Usage

For basic usage please see the [examples](/NetworkManager/examples) folder.

To use the provided wifi manager html page you need to move the `wifimanager.html` file into a `data` folder in the root of your `pio` project.

> **Warning**: It is **required** to add a build flag to your setup for the code to function properly.

For `platformio`

```ini
build_flags = 
  -DASYNCWEBSERVER_REGEX ; add regex support to AsyncWebServer
```

For the Arduino IDE:

Create, if missing, or update the `platform.local.txt` file.

The paths are:

>Windows

```bash
Windows: C:\Users\(username)\AppData\Local\Arduino15\packages\espxxxx\hardware\espxxxx\{version}\platform.local.txt
```

>Linux

```bash
Linux: ~/.arduino15/packages/espxxxx/hardware/espxxxx/{version}/platform.local.txt
```

The text to add is:

```txt
compiler.cpp.extra_flags=-DASYNCWEBSERVER_REGEX=1
```

> **Note**: This library is still in development and is not yet complete, if there are any bugs please report them in the issues section.

## Modification

This library is intended to be modified and extended. If you find any bugs, please make a new issue and I will fix it.

If you have any questions, please ask in the [discussions](https://github.com/ZanzyTHEbar/EasyNetworkManager/discussions).

To extend any of the enums please use the `data/utilities/enuminheritance.hpp` file.

To extend any of the config sections, simply create a namespace with the same name as the config struct is in and add your own `struct` to it. For example, to extend the `DeviceConfig_t` `struct`, you would do the following:

```cpp
namespace Project_Config {

    struct NewConfig_t : DeviceConfig_t {
        String newConfig;
        int newint;
        bool newbool;
    };
}

Project_Config::NewConfig_t newConfig; // this creates a new object of your config struct.
```



## Extras

To see any of the `log` statements - you need to add this to your `platformio.ini`:

```ini
build_flags = 
  -DCORE_DEBUG_LEVEL=4 ; add verbose debug logging in serial monitor

; other build parameters
monitor_filters = 
	esp32_exception_decoder
build_type = debug
lib_ldf_mode = deep+
board_build.partitions = min_spiffs.csv ; use min_spiffs partition table for a large WebServer App - or huge_app.csv for a large Code-based App
```

If you want to build in debug mode add this (it's not a build flag):

```ini
build_type = debug
```

## License

This library is licensed under the MIT License.
