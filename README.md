# EasyNetworkManager Library

> [!NOTE]\
> A documentation website is being built for this library, you canfind it here [Documentation](https://zanzythebar.github.io/EasyNetworkManager-Docs).\
> Please feel free to ask me any questions in the [repo discussion forum](https://github.com/ZanzyTHEbar/EasyNetworkManager/discussions).\
> To see an example `Platformio` project that uses this library: <https://github.com/ZanzyTHEbar/WaterChamber>\
> To see an example of a widly-used production project that uses this library: <https://github.com/EyeTrackVR/OpenIris>

This is an in-progress library for easy network management.

> [!IMPORTANT]\
> This library requires c++17
> see [extras](#extras)

This project supports the following boards:

- ESP8266
- ESP32

> [!NOTE]\
> Full ESP32C3 support is still in development, please report any bugs in the issues section.
> Of note, this has been successfully tested on all boards except for the ESP32C3.
> This library fully supports M5Stack devices.

This library provides a WiFi Manager front-end on a customisable URL endpoint using a provided HTML file.

![WiFi Manager](/assets/images/wifimanager.png)

It also provides numerous key features such as:

- autodiscovery of saved networks
- saving networks to memory
- automatically creating an Access Point if connecting to a wifi network fails
- mDNS
- Async OTA
- OTA
- customisable REST API
- WebSockets

And much more :smile: See the classes below.

This library implements the following classes:

- APIServer - A server that can be used to manage asynchronous REST API methods.
  - has a `handleJSON` method for handling `POST` and `GET` requests. Can send and receive JSON.
  - has a built-in async-ota endpoint that is disabled by default

> [!NOTE]\
> `POST` requests for `JSON` are still in development.

- WiFiHandler - A class that can be used to manage WiFi connections.
- OTA - A basic OTA handler.
- MDNSHandler - A class that can be used to manage mDNS services.
- Config - A class that can be used to manage configuration files.
- StateManager - A class that can be used to manage the state of your project (very easy to extend).
- Utilities - A folder that has various template files that can be used to manage various utilities.

## Installation

### Platformio (recommended)

You can install via the `Platformio Registry` by navigating to the `Libraries` section of `Platformio`.
The library is called `EasyNetworkManager` by `ZanzyTHEbar`.

if you like to install the bleading edge, in your `platformio.ini` file add the following:

```ini
lib_deps =
    https://github.com/ZanzyTHEbar/EasyNetworkManager.git
```

#### Arduino IDE

To install this library in your Arduino IDE, you must add all dependencies (sorry) and then download this repository as a zip file and it as any other library :smile:.

## Dependencies

All dependencies _should_ be installed automatically. If not, please make a new issue and I will fix it.

> [!NOTE]\
> `ESP8266` support is still in beta, for now you manually have to install the dependancies listed below.
> `ESPAsyncTCP`
> You _may_ need to install `ESP8266WiFi` if the compiler complains about it, but you shouldn't need to.

### Dependencies used in this project

- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer.git)

`ESP32`

- [AsyncTCP](https://github.com/me-no-dev/AsyncTCP.git)

`ESP8266`

- [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP)

## Usage

For basic usage please see the [examples](/NetworkManager/examples) folder.

To use the provided [wifi manager html](/NetworkManager/ui/wifimanager.html) page you don't need to do anything except for set a `define` before you include the library header (for `pio` users see the [Extras](#extras) section)

Then, build and flash the SPIFFS image as normal.

For the ArduinoIDE you will need to follow a tutorial on `SPIFFS` and flash any custom html files using `SPIFFS`.

> [!WARNING]\
> SPIFFS tools **do not** work yet in the ArduinoIDE 2.0. Support is coming soon.

### Runtime loop

Call `networkManager.loop()` once per iteration of your Arduino `loop()` function (see the [examples](/NetworkManager/examples)). Wi-Fi connecting is event-driven and non-blocking: `begin()` kicks off the first candidate network and returns immediately, while `loop()` enforces the per-attempt timeout, advances through the saved networks and fallback credentials, starts the AP fallback once all candidates are exhausted, and brings mDNS up as soon as the network is reachable. If you skip `loop()`, connection handling stalls.

The default per-attempt connect timeout is 15 seconds; override it by defining `EASYNETWORKMANAGER_WIFI_CONNECT_TIMEOUT_MS` (milliseconds) at build time. Timeouts are measured with wall-clock `millis()`, so a long-blocking user `loop()` stretches them proportionally — keep your `loop()` cooperative.

## Configuration

> [!WARNING]\
> It is **required** to add a build flag to your setup for the code to function properly.

For `platformio`

```ini
build_flags =
  -DASYNCWEBSERVER_REGEX ; add regex support to AsyncWebServer
```

Optionally you can enable the wifi manager and the async ota here as well:

```ini
build_flags =
  -DASYNCWEBSERVER_REGEX ; add regex support to AsyncWebServer
  -DUSE_WEBMANAGER ; enable wifimanager
  -DUSE_ASYNCOTA ; enable async ota support
```

### Management authentication

The API, web manager, and Async OTA endpoints share the configured OTA login
and password and reject requests when either credential is missing or invalid.
The basic Arduino OTA path also requires the configured OTA password. Secrets
are not included in configuration responses or logs. `APIServer` does not own
an `AsyncOTA*` passed to it; the caller must keep that object alive until the
API server is destroyed.

The current AsyncWebServer stack is plain HTTP and has no TLS
introspection/server. Authenticated management and OTA requests therefore
fail closed unless the application explicitly defines
`EASYNETWORKMANAGER_ALLOW_INSECURE_HTTP`. Use that flag only for a WPA2 AP
bootstrap, trusted development/Wokwi testing, or a deployment where a trusted
TLS boundary terminates HTTP before it reaches the device. The debug and Wokwi
PlatformIO environments enable it; release environments intentionally do not.
`EASYNETWORKMANAGER_ALLOW_INSECURE_OTA` is a separate development-only
signature bypass and does not enable HTTP authentication.

First boot can use the built-in `/provision` route. Leave both `ota_login` and
`ota_password` empty, start the device in AP mode with an 8-63 character WPA2
AP password that is not the legacy default `12345678`, and open `GET /provision`
from that AP. Submit a form `POST /provision` containing required `ssid`,
`password`, `ota_login`, and `ota_password` fields, plus optional `networkName`
and `channel` (1-14; defaults to 1). STA SSIDs and names are limited to 32
characters, STA passwords to 63 characters (empty is allowed for an open
network), and management credentials to 64 characters; management passwords
must be 8-64 characters. A successful request returns 202 after one atomic
`ProjectConfig::save()` and contains no secrets. Restart the device to use the
saved STA credentials. The route returns 404 outside this exact first-boot AP
state and is never available in STA/online mode. This WPA2-gated bootstrap
route is intentionally unauthenticated; subsequent plain-HTTP management and
OTA still require `EASYNETWORKMANAGER_ALLOW_INSECURE_HTTP`.

On ESP32, production Async OTA is disabled until the application configures a
PEM public key before calling `begin()`:

```cpp
if (!asyncOta.setPublicKey(OTA_PUBLIC_KEY_PEM)) {
    // Invalid or unsupported public key; do not start OTA.
}
```

`setPublicKey` copies only public material. Each production `POST /update`
must provide exactly one detached, standard-base64 `signature` before the file
body, using either the URL-encoded query parameter
`?signature=<base64-signature>` or the `X-OTA-Signature` header. The signature
must not be sent as a multipart field. The MD5 value must likewise be supplied
exactly once as the `MD5` query parameter or `X-OTA-MD5` header; neither value
is read from multipart body fields. The signature is made over the uploaded
image's SHA-256 digest (for example,
`openssl dgst -sha256 -sign ota-private.pem firmware.bin > firmware.sig`, then
base64-encode `firmware.sig`). MD5 remains an integrity check, not an
authenticity mechanism.

Async OTA and Arduino OTA can be enabled without signing only by explicitly
defining `EASYNETWORKMANAGER_ALLOW_INSECURE_OTA`; this development-only opt-in
skips the signature requirement/verification but still requires the
deterministic MD5 query/header input. It still requires
`EASYNETWORKMANAGER_ALLOW_INSECURE_HTTP` for the current plain HTTP stack and
must not be used on untrusted networks.
The signed implementation is unavailable on ESP8266, so production Async OTA
fails closed there; the insecure opt-in is the only legacy development path.
Signing authenticates the image but does not encrypt HTTP or protect
Basic-auth credentials in transit; deploy TLS separately. No anti-rollback or
freshness metadata is enforced, so an older validly signed image remains
acceptable.

For `ArduinoIDE`:

Create, if missing, or update the `platform.local.txt` file.

The paths are:

> Windows

```bash
Windows: C:\Users\(username)\AppData\Local\Arduino15\packages\espxxxx\hardware\espxxxx\{version}\platform.local.txt
```

> Linux

```bash
Linux: ~/.arduino15/packages/espxxxx/hardware/espxxxx/{version}/platform.local.txt
```

The text to add is:

```txt
compiler.cpp.extra_flags=-DASYNCWEBSERVER_REGEX=1
```

Optionally you can enable the wifi manager here as well:

```txt
compiler.cpp.extra_flags=-DASYNCWEBSERVER_REGEX=1 -DUSE_WEBMANAGER=1
```

> [!WARNING]\
> This library is still in development, if there are any bugs please report them in the issues section.

## Modification

This library is intended to be modified and extended. If you find any bugs, please make a new issue and I will fix it.

If you have any questions, please ask in the [discussions](https://github.com/ZanzyTHEbar/EasyNetworkManager/discussions).

To extend any of the enums please use the `data/utilities/enuminheritance.hpp` file.

To use your own custom config, simply inherit from the `CustomConfigInterface`, override the `save` and `load` methods,and then register your config.

```cpp
ConfigHandler configHandler("baseConf", MDNS_HOSTNAME);

class CustomConfig : public CustomConfigInterface {
    void save() override {
        this->log("Saving custom config");
    }

    void load() override {
        otherStuff();
        this->log("Loading custom config");
    }
    void otherStuff() {
      // do stuff
    }
};

CustomConfig customConfig;

// pass our custom config to the base config so that it can use it.
configHandler.config.registerUserConfig(&customConfig);
```

## Extras

To see any of the `log` statements used in this library - you need to add this to your `platformio.ini`:

```ini
build_flags =
  -DASYNCWEBSERVER_REGEX # add regex support to AsyncWebServer
  -DUSE_WEBMANAGER # enable wifimanager
  -DCORE_DEBUG_LEVEL=4 # add debug logging in serial monitor
  -std=gnu++17
build_unflags = -std=gnu++11

; other build parameters
monitor_filters =
 esp32_exception_decoder
build_type = debug
lib_ldf_mode = deep
```

If you want to build in debug mode add this (it's not a build flag):

```ini
build_type = debug
```

## License

This library is licensed under the MIT License.
