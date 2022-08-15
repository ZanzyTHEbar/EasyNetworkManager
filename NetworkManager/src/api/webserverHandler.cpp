#include "webserverHandler.hpp"

//! These have to be called before the constructor of the class because they are static
//! C++ 11 does not have inline variables, sadly. So we have to do this.
const char *APIServer::MIMETYPE_HTML{"text/html"};
// const char *APIServer::MIMETYPE_CSS{"text/css"};
// const char *APIServer::MIMETYPE_JS{"application/javascript"};
// const char *APIServer::MIMETYPE_PNG{"image/png"};
// const char *APIServer::MIMETYPE_JPG{"image/jpeg"};
// const char *APIServer::MIMETYPE_ICO{"image/x-icon"};
const char *APIServer::MIMETYPE_JSON{"application/json"};

bool APIServer::ssid_write = false;
bool APIServer::pass_write = false;
bool APIServer::channel_write = false;

//*********************************************************************************************
//!                                     API Server
//*********************************************************************************************

APIServer::APIServer(int CONTROL_PORT, WiFiHandler *network, std::string api_url, std::string wifimanager_url) : network(network),
																												 server(new AsyncWebServer(CONTROL_PORT)),
																												 api_url(api_url),
																												 wifimanager_url(wifimanager_url) {}

void APIServer::begin()
{
	this->setupServer();
	//! i have changed this to use lambdas instead of std::bind to avoid the overhead. Lambdas are always more preferable.
	server->on("/", HTTP_GET, [&](AsyncWebServerRequest *request)
			   { request->send(200); });

	if (api_utilities.initSPIFFS())
	{
		server->on(wifimanager_url.c_str(), HTTP_GET, [&](AsyncWebServerRequest *request)
				   { request->send(SPIFFS, "/wifimanager.html", MIMETYPE_HTML); });

		server->serveStatic(wifimanager_url.c_str(), SPIFFS, "/");
	}

	// preflight cors check
	server->on("/", HTTP_OPTIONS, [&](AsyncWebServerRequest *request)
			   {
        AsyncWebServerResponse* response = request->beginResponse(204);
        response->addHeader("Access-Control-Allow-Methods", "PUT,POST,GET,OPTIONS");
        response->addHeader("Access-Control-Allow-Headers", "Accept, Content-Type, Authorization, FileSize");
        response->addHeader("Access-Control-Allow-Credentials", "true");
        request->send(response); });

	DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

	// std::bind(&APIServer::API_Utilities::notFound, &api_utilities, std::placeholders::_1);
	server->onNotFound([&](AsyncWebServerRequest *request)
					   { api_utilities.notFound(request); });
	this->server->on(api_url.c_str(), HTTP_GET, [&](AsyncWebServerRequest *request)
					 { command_handler(request); });

	log_d("Initializing REST API");
	this->server->begin();
}

void APIServer::findParam(AsyncWebServerRequest *request, const char *param, String &value)
{
	if (request->hasParam(param))
	{
		value = request->getParam(param)->value();
	}
}

void APIServer::setupServer()
{
	command_map_wifi_conf.emplace("ssid", [this](const char *value) -> void
								  { setSSID(value); });
	command_map_wifi_conf.emplace("password", [this](const char *value) -> void
								  { setPass(value); });
	command_map_wifi_conf.emplace("channel", [this](const char *value) -> void
								  { setChannel(value); });

	command_map_funct.emplace("reboot_device", [this](void) -> void
							  { rebootDevice(); });
	command_map_funct.emplace("reset_config", [this](void) -> void
							  { factoryReset(); });

	command_map_json.emplace("data_json", [this](AsyncWebServerRequest *request) -> void
							 { setDataJson(request); });
	command_map_json.emplace("config_json", [this](AsyncWebServerRequest *request) -> void
							 { setConfigJson(request); });
	command_map_json.emplace("settings_json", [this](AsyncWebServerRequest *request) -> void
							 { setSettingsJson(request); });
}

void APIServer::addCommandHandler(std::string index, function func)
{
	command_map_funct.emplace(index, [&](void) -> void
							  { func(); });
}

void APIServer::command_handler(AsyncWebServerRequest *request)
{
	int params = request->params();
	for (int i = 0; i < params; i++)
	{
		AsyncWebParameter *param = request->getParam(i);
		{
			command_map_wifi_conf_t::const_iterator it_wifi_conf = command_map_wifi_conf.find(param->name().c_str());
			command_map_funct_t::const_iterator it_funct = command_map_funct.find(param->name().c_str());
			command_map_json_t::const_iterator it_json = command_map_json.find(param->name().c_str());

			if (it_wifi_conf != command_map_wifi_conf.end())
			{
				command_map_wifi_conf.at(param->name().c_str())(param->value().c_str());
				auto &key_it = it_wifi_conf->first;
				log_i("Command %s executed", key_it.c_str());
			}
			else if (it_funct != command_map_funct.end())
			{
				command_map_funct.at(param->name().c_str())();
				auto &key_it_funct = it_funct->first;
				log_i("Command %s executed", key_it_funct.c_str());
			}
			else if (it_json != command_map_json.end())
			{
				command_map_json.at(param->name().c_str())(request);
				auto &key_it_json = it_json->first;
				log_i("Command %s executed", key_it_json.c_str());
			}
			else
			{
				log_i("Command not found");
			}
		}
		log_i("GET[%s]: %s\n", param->name().c_str(), param->value().c_str());
	}
}

//*********************************************************************************************
//!                                     Command Functions
//*********************************************************************************************
void APIServer::setSSID(const char *value)
{
	if (network->stateManager->getCurrentState() == WiFiState_e::WiFiState_ADHOC)
		this->wifiConfig.local_WifiConfig[0].ssid = value;
	else
		this->wifiConfig.local_WifiConfig[1].ssid = value;
	ssid_write = true;
}

void APIServer::setPass(const char *value)
{
	if (network->stateManager->getCurrentState() == WiFiState_e::WiFiState_ADHOC)
		this->wifiConfig.local_WifiConfig[0].pass = value;
	else
		this->wifiConfig.local_WifiConfig[1].pass = value;
	pass_write = true;
}

void APIServer::setChannel(const char *value)
{
	if (network->stateManager->getCurrentState() == WiFiState_e::WiFiState_ADHOC)
		this->wifiConfig.local_WifiConfig[0].channel = atoi(value);
	else
		this->wifiConfig.local_WifiConfig[1].channel = atoi(value);
	channel_write = true;
}

/**
 * * Trigger in main loop to save config to flash
 * ? Should we force the users to update all config params before triggering a config write?
 */
void APIServer::triggerWifiConfigWrite()
{
	if (ssid_write && pass_write && channel_write)
	{
		ssid_write = false;
		pass_write = false;
		channel_write = false;
		if (network->stateManager->getCurrentState() == WiFiState_e::WiFiState_ADHOC)
			network->configManager->setWifiConfig(wifiConfig.local_WifiConfig[0].ssid.c_str(), wifiConfig.local_WifiConfig[0].ssid.c_str(), wifiConfig.local_WifiConfig[0].pass.c_str(), &wifiConfig.local_WifiConfig[0].channel, true);
		else
			network->configManager->setWifiConfig(wifiConfig.local_WifiConfig[1].ssid.c_str(), wifiConfig.local_WifiConfig[1].ssid.c_str(), wifiConfig.local_WifiConfig[1].pass.c_str(), &wifiConfig.local_WifiConfig[1].channel, true);
		network->configManager->save();
	}
}

void APIServer::setDataJson(AsyncWebServerRequest *request)
{
	network->configManager->getDeviceConfig()->data_json = true;
	Network_Utilities::my_delay(1L);
	String temp = network->configManager->getDeviceConfig()->data_json_string;
	request->send(200, MIMETYPE_JSON, temp);
	temp = "";
}

void APIServer::setConfigJson(AsyncWebServerRequest *request)
{
	network->configManager->getDeviceConfig()->config_json = true;
	Network_Utilities::my_delay(1L);
	String temp = network->configManager->getDeviceConfig()->config_json_string;
	request->send(200, MIMETYPE_JSON, temp);
	temp = "";
}

void APIServer::setSettingsJson(AsyncWebServerRequest *request)
{
	network->configManager->getDeviceConfig()->settings_json = true;
	Network_Utilities::my_delay(1L);
	String temp = network->configManager->getDeviceConfig()->settings_json_string;
	request->send(200, MIMETYPE_JSON, temp);
	temp = "";
}

void APIServer::rebootDevice()
{
	delay(20000);
	ESP.restart();
}

void APIServer::factoryReset()
{
	network->configManager->reset();
}

//*********************************************************************************************
//!                                     API Utilities
//*********************************************************************************************

APIServer::API_Utilities::API_Utilities() {}

std::string APIServer::API_Utilities::shaEncoder(std::string data)
{
	const char *data_c = data.c_str();
	int size = 20;
	uint8_t hash[size];
	mbedtls_md_context_t ctx;
	mbedtls_md_type_t md_type = MBEDTLS_MD_SHA1;

	const size_t len = strlen(data_c);
	mbedtls_md_init(&ctx);
	mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
	mbedtls_md_starts(&ctx);
	mbedtls_md_update(&ctx, (const unsigned char *)data_c, len);
	mbedtls_md_finish(&ctx, hash);
	mbedtls_md_free(&ctx);

	std::string hash_string = "";
	for (uint16_t i = 0; i < size; i++)
	{
		std::string hex = String(hash[i], HEX).c_str();
		if (hex.length() < 2)
		{
			hex = "0" + hex;
		}
		hash_string += hex;
	}
	return hash_string;
}

void APIServer::API_Utilities::notFound(AsyncWebServerRequest *request)
{
	try
	{
		log_i("%s", _networkMethodsMap[request->method()]);
	}
	catch (const std::exception &e)
	{
		log_i("UNKNOWN");
	}

	log_i(" http://%s%s/\n", request->host().c_str(), request->url().c_str());
	request->send(404, "text/plain", "Not found.");
}

// Initialize SPIFFS
bool APIServer::API_Utilities::initSPIFFS()
{
	bool init_spiffs = SPIFFS.begin(false);
	log_e("[SPIFFS]: SPIFFS Initialized: %s", init_spiffs ? "true" : "false");
	return init_spiffs;
}

// Read File from SPIFFS
String APIServer::API_Utilities::readFile(fs::FS &fs, std::string path)
{
	log_i("Reading file: %s\r\n", path.c_str());

	File file = fs.open(path.c_str());
	if (!file || file.isDirectory())
	{
		log_e("[INFO]: Failed to open file for reading");
		return String();
	}

	String fileContent;
	while (file.available())
	{
		fileContent = file.readStringUntil('\n');
		break;
	}
	return fileContent;
}

// Write file to SPIFFS
void APIServer::API_Utilities::writeFile(fs::FS &fs, std::string path, std::string message)
{
	log_i("[Writing File]: Writing file: %s\r\n", path);
	Network_Utilities::my_delay(0.1L);

	File file = fs.open(path.c_str(), FILE_WRITE);
	if (!file)
	{
		log_i("[Writing File]: failed to open file for writing");
		return;
	}
	if (file.print(message.c_str()))
	{
		log_i("[Writing File]: file written");
	}
	else
	{
		log_i("[Writing File]: file write failed");
	}
}

APIServer::API_Utilities api_utilities;