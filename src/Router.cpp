#include "../incl/Router.hpp"

Router::Router(const Server42& allServersConfig): allServers_(allServersConfig)
{}
ActionParameters Router::routeRequest(const Request& request, int listeningPort) const
{}

const SingleServer* Router::selectServerBlock(const Request& request, int listeningPort) const
{
	const SingleServer* selectedServer = nullptr;
	const SingleServer* defaultServer = nullptr;
	std::string requestHostHeader;

	auto hostIt = request.getHeaders().find("Host");
	if (hostIt != request.getHeaders().end())
	{
		requestHostHeader = hostIt->second;
	} else{
		// in the Request parsing we already checked it
		return nullptr;
	}
	std::string hostWithoutPort = requestHostHeader;
	size_t posColon = hostWithoutPort.find(':');
	if (posColon != std::string::npos) {
		hostWithoutPort = hostWithoutPort.substr(0, posColon);
	}
	const std::vector<SingleServer> &servers = allServers_.getServers();
	for (size_t i = 0; i < servers.size(); i++)
	{
		const SingleServer &server = servers[i];
		if (server.getServPortInt() == listeningPort){
			if(!defaultServer){
				defaultServer = &server;
			}
		}
		if (server.getServName() == hostWithoutPort){
			selectedServer = &server;
			break;
		}
	}
	if (!selectedServer)
		selectedServer = defaultServer;
	return selectedServer;
}

const Location* Router::findBestMatchingLocation(const Request& request, const SingleServer* server) const
{
	const Location* matchedLocation = nullptr;
	size_t longestMatchLength = 0;
	
	const std::vector<Location> &locations = server->getLocations();
	std::string requestPath = request.getPath();
	for(int i = 0; i < locations.size(); i++){
		const Location &location = locations[i];
		const std::string &locationPath = location.getPath();
		// TODO:double check!!
		if (requestPath.rfind(locationPath, 0) == 0) {
			if (locationPath.length() > longestMatchLength) {
				longestMatchLength = locationPath.length();
				matchedLocation = &location;
			}
		}
	}
	return matchedLocation;
}

ActionParameters Router::determineAction(const Request& request, const SingleServer* selectedServer, const Location* selectedLocation) const
{
	ActionParameters params;
	params.matchedServer = selectedServer;
	params.matchedLocation = selectedLocation;

	// Methods Allowed
	std::vector<std::string> methods = selectedLocation->getAllowedMethods();
	bool findAllowedMethod = false;
	for (int i = 0; i < methods.size(); i++){
		if (methods[i] == request.getMethod()){
			findAllowedMethod = true;
			break;
		}
	}
	if (!findAllowedMethod){
		params.errorCode = 405;
		return params;
	}
	// Redirect
	if (selectedLocation->getRedirectionCode() != -1 && !selectedLocation->getRedirectionsPath().empty()) {
		params.isRedirect = true;
		params.redirectCode = selectedLocation->getRedirectionCode();
		params.redirectUrl = selectedLocation->getRedirectionsPath();
		return params;
	}
	// Max body limit
	if (request.getBody().length() > (size_t)selectedServer->getMaxBodySize() && selectedServer->getMaxBodySize() != -1)
	{
		params.errorCode = 413;
		return params;
	}
}


bool Router::fileExists(const std::string& path) const
{}
bool Router::isDirectory(const std::string& path) const
{}
