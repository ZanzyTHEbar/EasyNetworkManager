#include "webserverHandler.hpp"

std::vector<BaseAPI::stateFunctionRow_t> APIServer::stateFunctionRows = {{"", NULL}};

//*********************************************************************************************
//!                                     API Server
//*********************************************************************************************

APIServer::APIServer(int CONTROL_PORT,
					 WiFiHandler *network,
					 DNSServer *dnsServer,
					 std::string api_url,
					 std::string wifimanager_url,
					 std::string userCommands) : BaseAPI(CONTROL_PORT,
														 network,
														 dnsServer,
														 api_url,
														 wifimanager_url,
														 userCommands) {}

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

	char buff[1000];
	snprintf(buff, sizeof(buff), "^\\%s\\/command\\/([a-zA-Z0-9]+)$", this->userCommands.c_str());
	server->on(buff, HTTP_ANY, [&](AsyncWebServerRequest *request)
			   { handleUserCommands(request); });

	server->begin();
}

void APIServer::setupServer()
{
	// Set default routes
	routes.emplace("wifi", &APIServer::setWiFi);
	routes.emplace("reset_config", &APIServer::factoryReset);
	routes.emplace("reboot_device", &APIServer::rebootDevice);
	routes.emplace("set_json", &APIServer::handleJson);

	routeHandler("builtin", routes); // add new map to the route map
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
	log_i("Request URL: %s", request->url().c_str());
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

void APIServer::updateCommandHandlers(std::string index, stateFunction_t funct)
{
	stateFunctionRows.emplace_back(index, funct);
}

void APIServer::handleUserCommands(AsyncWebServerRequest *request)
{
	for (const auto &member : stateFunctionRows)
	{
		if (member.name == request->pathArg(0).c_str())
		{
			log_i("User Command Executed: %s", request->pathArg(0).c_str());
			// Call the function
			member.func();
			return;
		}
		else
		{
			log_i("User Command Not Found: %s", request->pathArg(0).c_str());
		}
	}
}