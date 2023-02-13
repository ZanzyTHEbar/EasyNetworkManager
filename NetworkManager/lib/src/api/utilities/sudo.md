# API Handler

AnyApi (represented through baseAPI)

    - Setup
        - Saves all api functions in map <String, function> (maybe wrap before?)
        - Calls webserverHandler to save routes in route_map

webserverHandler

    - Setup
        - Start webserver, handle base "/" request
    - Function
        - Send request to specific API through map<string,map<string,function>>
        - Somehow switch-case

apiUtilities

    - might wrap functions to better api-function
