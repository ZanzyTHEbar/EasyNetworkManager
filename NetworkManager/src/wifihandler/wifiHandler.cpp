#include "WifiHandler.hpp"
#include <vector>

WiFiHandler::WiFiHandler(ProjectConfig *configManager,
						 StateManager<WiFiState_e> *stateManager,
						 const std::string &ssid,
						 const std::string &password,
						 const std::string &hostname,
						 uint8_t channel) : configManager(configManager),
											stateManager(stateManager),
											ssid(ssid),
											password(password),
											_hostname(hostname),
											channel(channel),
											_enable_adhoc(false) {}

WiFiHandler::~WiFiHandler() {}

void WiFiHandler::setupWifi()
{
	if (this->_enable_adhoc || stateManager->getCurrentState() == WiFiState_e::WiFiState_ADHOC)
	{
		this->setUpADHOC();
		return;
	}

	log_i("Initializing connection to wifi");
	stateManager->setState(WiFiState_e::WiFiState_Connecting);

	std::vector<Project_Config::WiFiConfig_t> *networks = configManager->getWifiConfigs();

	// check size of networks
	log_i("Found %d networks", networks->size());
	if (networks->size() == 0)
	{
		log_e("No networks found in config");
		stateManager->setState(WiFiState_e::WiFiState_Error);
		this->iniSTA();
		return;
	}

	int connection_timeout = 30000; // 30 seconds

	int count = 0;
	unsigned long currentMillis = millis();
	unsigned long _previousMillis = currentMillis;
	int progress = 0;
	WiFi.mode(WIFI_STA);
	WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
	WiFi.setHostname(_hostname.c_str()); // define hostname
	for (auto networkIterator = networks->begin(); networkIterator != networks->end(); ++networkIterator)
	{
		log_i("Trying to connect to the %s network", networkIterator->ssid.c_str());
		WiFi.begin(networkIterator->ssid.c_str(), networkIterator->password.c_str());
		count++;
		while (WiFi.status() != WL_CONNECTED)
		{
			progress++;
			stateManager->setState(ProgramStates::DeviceStates::WiFiState_e::WiFiState_Connecting);
			currentMillis = millis();
			Helpers::update_progress_bar(progress, 100);
			delay(301);
			if (((currentMillis - _previousMillis) >= connection_timeout) && (count >= networks->size()))
			{
				log_e("Connection to %s timed out", networkIterator->ssid);
				log_i("[INFO]: WiFi connection timed out.\n");
				// we've tried all saved networks, none worked, let's error out
				log_e("Could not connect to any of the saved networks, check your Wifi credentials");
				stateManager->setState(WiFiState_e::WiFiState_Disconnected);
				log_i("[INFO]: Attempting to connect to hardcoded network");
				this->iniSTA();
				return;
			}
		}
		if (!WiFi.isConnected())
			log_i("\n\rCould not connect to %s, trying another network\n\r", networkIterator->ssid);
		else
		{
			log_i("\n\rSuccessfully connected to %s\n\r", networkIterator->ssid.c_str());
			stateManager->setState(WiFiState_e::WiFiState_Connected);
			return;
		}
	}
}

void WiFiHandler::adhoc(const char *ssid, const char *password, uint8_t channel)
{
	log_i("[INFO]: Setting Access Point...\n");

	log_i("[INFO]: Configuring access point...\n");
	WiFi.mode(WIFI_AP);

	Serial.printf("\r\nStarting AP. \r\nAP IP address: ");
	IPAddress IP = WiFi.softAPIP();
	Serial.printf("[INFO]: AP IP address: %s.\r\n", IP.toString().c_str());

	// You can remove the password parameter if you want the AP to be open.
	WiFi.softAP(ssid, password, channel); // AP mode with password
	WiFi.setTxPower(WIFI_POWER_11dBm);

	stateManager->setState(WiFiState_e::WiFiState_ADHOC);
}

/*
 * *
 */
void WiFiHandler::setUpADHOC()
{
	log_i("[INFO]: Setting Access Point...\n");
	size_t ssidLen = strlen(configManager->getAPWifiConfig()->ssid.c_str());
	size_t passwordLen = strlen(configManager->getAPWifiConfig()->password.c_str());
	char ssid[ssidLen + 1];
	char password[passwordLen + 1];
	uint8_t channel = configManager->getAPWifiConfig()->channel;
	if (ssidLen > 0 || passwordLen > 0)
	{
		strcpy(ssid, configManager->getAPWifiConfig()->ssid.c_str());
		strcpy(password, configManager->getAPWifiConfig()->password.c_str());
		channel = configManager->getAPWifiConfig()->channel;
	}
	else
	{
		strcpy(ssid, "EasyNetworkManager");
		strcpy(password, "12345678");
		channel = 1;
	}

	this->adhoc(ssid, password, channel);

	log_i("[INFO]: Configuring access point...\n");
	log_d("[DEBUG]: ssid: %s\n", ssid);
	log_d("[DEBUG]: password: %s\n", password);
	log_d("[DEBUG]: channel: %d\n", channel);
}

void WiFiHandler::iniSTA()
{
	log_i("[INFO]: Setting up station...\n");
	int connection_timeout = 30000; // 30 seconds
	unsigned long currentMillis = millis();
	unsigned long _previousMillis = currentMillis;
	int progress = 0;

	log_i("Trying to connect to the %s network", this->ssid.c_str());
	//  check size of networks
	if (this->ssid.size() == 0)
	{
		log_e("No networks passed into the constructor");
		stateManager->setState(WiFiState_e::WiFiState_Error);
		this->setUpADHOC();
		return;
	}
	WiFi.begin(this->ssid.c_str(), this->password.c_str(), this->channel);
	// WiFi.setTxPower(WIFI_POWER_11dBm);

	while (WiFi.status() != WL_CONNECTED)
	{
		progress++;
		stateManager->setState(ProgramStates::DeviceStates::WiFiState_e::WiFiState_Connecting);
		currentMillis = millis();
		Helpers::update_progress_bar(progress, 100);
		delay(301);
		if ((currentMillis - _previousMillis) >= connection_timeout)
		{
			log_i("[INFO]: WiFi connection timed out.\n");
			// we've tried all saved networks, none worked, let's error out
			log_e("Could not connect to any of the save networks, check your Wifi credentials");
			stateManager->setState(WiFiState_e::WiFiState_Error);
			this->setUpADHOC();
			log_w("Setting up adhoc mode");
			log_w("Please use adhoc mode and the app to set your WiFi credentials and reboot the device");
			stateManager->setState(WiFiState_e::WiFiState_ADHOC);
			return;
		}
	}

	if (!WiFi.isConnected())
		log_i("\n\rCould not connect to %s, please try another network\n\r", this->ssid.c_str());
	else
	{
		log_i("\n\rSuccessfully connected to %s\n\r", this->ssid.c_str());
		stateManager->setState(WiFiState_e::WiFiState_Connected);
		return;
	}
}

void WiFiHandler::toggleAdhoc(bool *enable)
{
	_enable_adhoc = enable;
}