#include "webserverHandler.hpp"

//*********************************************************************************************
//!                                     API Server
//*********************************************************************************************

APIServer::APIServer(int CONTROL_PORT,
					 WiFiHandler *network,
					 DNSServer *dnsServer,
					 std::string api_url,
					 std::string wifimanager_url) : BaseAPI(CONTROL_PORT, network, dnsServer, api_url, wifimanager_url) {}

APIServer::~APIServer() {}

void APIServer::begin()
{
	log_d("Initializing REST API");
	this->setupServer();
	BaseAPI::begin();

	char buffer[1000];
	snprintf(buffer, sizeof(buffer), "^\\%s\\/([a-zA-Z0-9]+)\\/command\\/([a-zA-Z0-9]+)$", this->api_url.c_str());
	log_d("API URL: %s", buffer);
	server->on(buffer, HTTP_ANY, [&](AsyncWebServerRequest *request)
			   { handleRequest(request); });

	char buf[1000];
	snprintf(buf, sizeof(buf), "^\\%s\\/([a-zA-Z0-9]+)\\/command\\/([a-zA-Z0-9]+)$", this->wifimanager_url.c_str());
	server->on(buf, HTTP_ANY, [&](AsyncWebServerRequest *request)
			   { handleRequest(request); });

	server->begin();
}

void APIServer::setupServer()
{
	// Set default routes
	routes.emplace("wifi", &APIServer::setWiFi);
	routes.emplace("reset_config", &APIServer::factoryReset);
	routes.emplace("reboot_device", &APIServer::rebootDevice);
	routes.emplace("set_json", &APIServer::handleJson);

	routeHandler("builtin", routes);  // add new map to the route map
}

void APIServer::findParam(AsyncWebServerRequest *request, const char *param, String &value)
{
	if (request->hasParam(param))
	{
		value = request->getParam(param)->value();
	}
}

/**
 * @brief Add a command handler to the API
 *
 * @param index
 * @param funct
 * @return \c vector<string> a list of the indexes of the command handlers
 */
std::vector<std::string> APIServer::routeHandler(std::string index, route_t route)
{
	route_map.emplace(index, route);
	std::vector<std::string> indexes;
	indexes.reserve(route.size());

	for (const auto &key : route)
	{
		indexes.push_back(key.first);
	}

	return indexes;
}

/**
 * @brief Add a command handler to the API
 *
 * @param index
 * @param request
 * @return \c vector<string> a list of the indexes of the command handlers
 */
void APIServer::routeHandler(std::string index, AsyncWebServerRequest *request)
{
	switch (_networkMethodsMap_enum[request->method()])
	{
	case DELETE:
	{
		route_map.erase(index);
		break;
	}
	default:
	{
		request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Request\"}");
		break;
	}
	}
}

void APIServer::handleRequest(AsyncWebServerRequest *request)
{
	// Get the route
	log_i("Request: %s", request->url().c_str());
	int params = request->params();
	auto it_map = route_map.find(request->pathArg(0).c_str());
	log_i("Request: %s", request->pathArg(0).c_str());
	auto it_method = it_map->second.find(request->pathArg(1).c_str());
	log_i("Request: %s", request->pathArg(1).c_str());

	for (int i = 0; i < params; i++)
	{
		AsyncWebParameter *param = request->getParam(i);
		{
			{
				if (it_map != route_map.end())
				{
					if (it_method != it_map->second.end())
					{
						(*this.*(it_method->second))(request);
					}
					else
					{
						request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Command\"}");
						request->redirect("/");
						return;
					}
				}
				else
				{
					request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Map Index\"}");
					request->redirect("/");
					return;
				}
			}
			log_i("%s[%s]: %s\n", _networkMethodsMap[request->method()].c_str(), param->name().c_str(), param->value().c_str());
		}
	}
	request->send(200, MIMETYPE_JSON, "{\"msg\":\"Command executed\"}");
}

/* void APIServer::command_handler(AsyncWebServerRequest *request)
{
	int params = request->params();
	for (int i = 0; i < params; i++)
	{
		AsyncWebParameter *param = request->getParam(i);
		{
			command_map_method_t::const_iterator it_method = command_map_method.find(param->name().c_str());
			// command_map_funct_t::const_iterator it_funct = command_map_funct.find(param->name().c_str());
			//  command_map_json_t::const_iterator it_json = command_map_json.find(param->name().c_str());

			if (it_method != command_map_method.end())
			{
				(*this.*(it_method->second))();
				log_i("Command %s executed", param->name().c_str());
				request->send(200);
			}
			else if (it_funct != command_map_funct.end())
			{
				(*(it_funct->second))();
				log_i("Command %s executed", param->name().c_str());
				request->send(200);
			}
			else
			{
				log_e("Command %s not found", param->name().c_str());
				request->send(404, MIMETYPE_JSON, "Error: Command not found");
			}
		}
	}
} */

/* void APIServer::wifi_command_handler(AsyncWebServerRequest *request)
{
	int params = request->params();
	for (int i = 0; i < params; i++)
	{
		AsyncWebParameter *param = request->getParam(i);
		command_map_wifi_conf_t::const_iterator it_wifi_funct = command_map_wifi_conf.find(param->name().c_str());
		{
			if (it_wifi_funct != command_map_wifi_conf.end())
			{
				(*this.*(it_wifi_funct->second))(param->value().c_str());
				log_i("Command %s executed", param->name().c_str());
			}
			else
			{
				log_i("Command not found");
			}
		}
		log_i("%s[%s]: %s\n", _networkMethodsMap[request->method()].c_str(), param->name().c_str(), param->value().c_str());
	}
} */
