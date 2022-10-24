#include <Arduino.h>

//? Here is a list of all the library header files - required ones are marked with an asterisk (*)

//! Optional header files
#include <mDNS/MDNSManager.hpp>
#include <ota/OTA.hpp>
//#include <memory>
//#include <data/utilities/Observer.hpp>
//#include <api/utilities/apiUtilities.hpp>
//#include <data/utilities/enuminheritance.hpp> // used for extending enums with new values
//#include <data/utilities/makeunique.hpp> // used with smart pointers (unique_ptr) to create unique objects
//#include <data/utilities/helpers.hpp> // various helper functions
#include <data/utilities/network_utilities.hpp> // various network utilities
//#include <wifihandler/utilities/utilities.hpp> // various wifi related utilities

//! Required header files
#include <data/config/project_config.hpp>	  //! (*)
#include <data/StateManager/StateManager.hpp> //! (*)
#include <wifihandler/wifiHandler.hpp>		  //! (*)
#include <api/webserverHandler.hpp>			  //! (*)

//ProjectConfig configManager; // no custom names
//ProjectConfig configManager("config", std::string(), ); // custom config name
ProjectConfig configManager(std::string(), "app_name_here"); // custom MDNS name
WiFiHandler network(&configManager, &wifiStateManager, "", "", "_easynetwork", 1);

APIServer server(80, &configManager, NULL, "/api/v1", "/wifimanager", "/userCommands");
OTA ota(&configManager);
MDNSHandler mDNS(&mdnsStateManager, &configManager, "_easynetwork", "test", "_tcp", "api_port", "80"); //! service name and service protocol have to be lowercase and begin with an underscore

void printHelloWorld()
{
	Serial.println("Hello World!");
}

void blink()
{
	digitalWrite(27, HIGH);
	Network_Utilities::my_delay(0.5); // delay for 500ms
	digitalWrite(27, LOW);
	Network_Utilities::my_delay(0.5); // delay for 500ms
}

void setup()
{
	Serial.begin(115200);
	Serial.println("Hello, EasyNetworkManager!");

	Serial.setDebugOutput(true);
	configManager.initConfig(); // call before load to initialise the structs
	configManager.load();		// load the config from flash

	network.setupWifi(); // setup wifi connection
	mDNS.startMDNS();	 // start mDNS service (optional)

	// handle the WiFi connection state changes
	switch (wifiStateManager.getCurrentState())
	{
	case WiFiState_e::WiFiState_Disconnected:
	{
		break;
	}
	case WiFiState_e::WiFiState_Disconnecting:
	{
		break;
	}
	case WiFiState_e::WiFiState_ADHOC:
	{
		// only start the API server if we have wifi connection
		server.updateCommandHandlers("blink", blink);				 // add a command handler to the API server - you can add as many as you want - you can also add methods.
		server.updateCommandHandlers("helloWorld", printHelloWorld); // add a command handler to the API server - you can add as many as you want - you can also add methods.
		server.begin();
		log_d("[SETUP]: Starting API Server");
		break;
	}
	case WiFiState_e::WiFiState_Connected:
	{
		// only start the API server if we have wifi connection
		server.updateCommandHandlers("blink", blink);				 // add a command handler to the API server - you can add as many as you want - you can also add methods.
		server.updateCommandHandlers("helloWorld", printHelloWorld); // add a command handler to the API server - you can add as many as you want - you can also add methods.
		server.begin();
		log_d("[SETUP]: Starting API Server");
		break;
	}
	case WiFiState_e::WiFiState_Connecting:
	{
		break;
	}
	case WiFiState_e::WiFiState_Error:
	{
		break;
	}
	}
	ota.SetupOTA();
}

void loop()
{
	ota.HandleOTAUpdate();
}
