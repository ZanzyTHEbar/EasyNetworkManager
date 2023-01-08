#include "webserverHandler.hpp"

//*********************************************************************************************
//!                                     API Server
//*********************************************************************************************

APIServer::APIServer(int CONTROL_PORT,
					 ProjectConfig *configManager,
					 const std::string &api_url,
					 const std::string &wifimanager_url,
					 const std::string &userCommands) : BaseAPI(CONTROL_PORT,
																configManager,
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
	server.on(buffer, HTTP_ANY, [&](AsyncWebServerRequest *request)
			   { handleRequest(request); });

	char buf[1000];
	snprintf(buf, sizeof(buf), "^\\%s\\/([a-zA-Z0-9]+)\\/command\\/([a-zA-Z0-9]+)$", this->wifimanager_url.c_str());
	server.on(buf, HTTP_ANY, [&](AsyncWebServerRequest *request)
			   { handleRequest(request); });
	server.begin();
}

void APIServer::setupServer()
{
	// Set default routes
	routes.emplace("wifi", &APIServer::setWiFi);
	routes.emplace("json", &APIServer::handleJson);
	routes.emplace("resetConfig", &APIServer::factoryReset);
	routes.emplace("getConfig", &APIServer::getJsonConfig);
	routes.emplace("deleteRoute", &APIServer::removeRoute);
	routes.emplace("rebootDevice", &APIServer::rebootDevice);
	routes.emplace("ping", &APIServer::ping);
	routes.emplace("save", &APIServer::save);

	//! reserve enough memory for all routes - must be called after adding routes and before adding routes to route_map
	indexes.reserve(routes.size());			 // this is done to avoid reallocation of memory and copying of data
	addRouteMap("builtin", routes, indexes); // add new route map to the route_map
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
 * @param indexes \c std::vector<std::string> a list of the routes of the command handlers
 *
 * @return void
 *
 */
void APIServer::addRouteMap(const std::string &index, route_t route, std::vector<std::string> &indexes)
{
	route_map.emplace(std::move(index), route);

	for (const auto &key : route)
	{
		indexes.emplace_back(key.first); // add the route to the list of routes - use emplace_back to avoid copying
	}
}

void APIServer::handleRequest(AsyncWebServerRequest *request)
{
	std::vector<std::string> temp = Helpers::split(userCommands.c_str(), '/');

	if (strcmp(request->pathArg(0).c_str(), temp[1].c_str()) == 0)
	{
		handleUserCommands(request);
		return;
	}

	// Get the route
	log_i("Request URL: %s", request->url().c_str());
	log_i("Request: %s", request->pathArg(0).c_str());
	log_i("Request: %s", request->pathArg(1).c_str());

	auto it_map = route_map.find(request->pathArg(0).c_str());
	auto it_method = it_map->second.find(request->pathArg(1).c_str());

	if (it_map != route_map.end())
	{
		if (it_method != it_map->second.end())
		{
			log_d("We are trying to execute the function");
			(*this.*(it_method->second))(request);
		}
		else
		{
			log_e("Invalid Command");
			request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Command\"}");
			return;
		}
	}
	else
	{
		log_e("Invalid Map Index");
		request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Map Index\"}");
		return;
	}
}

void APIServer::updateCommandHandlers(const std::string &url, stateFunction_t funct)
{
	stateFunctionMap.emplace(std::move(url), funct);
}

void APIServer::updateCommandHandlers(const std::string &url, stateFunction_request_t funct)
{
	stateFunctionMapRequest.emplace(std::move(url), funct);
}

/**
 * @brief Add a command handler to the API
 *
 * @param request
 * @return \c void
 * @note This function is called by the API server when a command is received
 * @warning  \c This function requries the user to access the index using a url parameter \c we need to fix this!! I need a better implemenation
 *
 */
void APIServer::handleUserCommands(AsyncWebServerRequest *request)
{
	std::string url = request->pathArg(1).c_str();
	auto it = stateFunctionMap.find(url);
	if (it != stateFunctionMap.end())
	{
		(*it->second)();
	}
	else
	{
		request->send(400, MIMETYPE_JSON, "{\"msg\":\"Invalid Command\"}");
		request->redirect("/");
		return;
	}
}