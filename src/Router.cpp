#include "../incl/Router.hpp"

Router::Router(const Server42& allServersConfig): allServers_(allServersConfig)
{}

Router::~Router(){}

ActionParameters Router::routeRequest(const Request& request, int listeningPort) const
{
	ActionParameters params;
	const SingleServer *selectedServer = selectServerBlock(request,listeningPort);
	if (!selectedServer){
		params.errorCode = 500;
		return params;
	}
	params.matchedServer = selectedServer;
	const Location *selectedLocation = findBestMatchingLocation(request, selectedServer);
	if(!selectedLocation){
		params.errorCode = 404;
		return params;
	}
	params.matchedLocation = selectedLocation;
	params = determineAction(request, params.matchedServer, params.matchedLocation);
	if (params.errorCode != 0 && params.matchedServer){
		const std::unordered_map<int, std::string>	&errorPages = params.matchedServer->getErrorPages();
		std::unordered_map<int, std::string>::const_iterator it = errorPages.find(params.errorCode);
		if (it != errorPages.end()){
			params.errorPagePath = it->second;
		}
	}
	return params;
}

const SingleServer* Router::selectServerBlock(const Request& request, int listeningPort) const
{
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
	const std::vector<std::shared_ptr<SingleServer>>& servers = allServers_.getServers();
	for (size_t i = 0; i < servers.size(); i++)
	{
		const SingleServer *server = servers[i].get();
		if (server->getServPortInt() == listeningPort){
			if (server->getServName() == hostWithoutPort){
				return server;
			}
			if(!defaultServer){
				defaultServer = server;
			}
		}
	}
	return defaultServer;
}

const Location* Router::findBestMatchingLocation(const Request& request, const SingleServer* server) const
{
	const Location* matchedLocation = nullptr;
	size_t longestMatchLength = 0;
	
	const std::vector<std::shared_ptr<Location>>& locations = server->getLocations();
	std::string requestPath = request.getPath();

	for(int i = 0; i < (int)locations.size(); i++){
		const std::shared_ptr<Location>& location = locations[i];
		const std::string &locationPath = location->getPath();
		if (requestPath.find(locationPath, 0) == 0) {
			if (locationPath.length() > longestMatchLength) {
				longestMatchLength = locationPath.length();
				matchedLocation = location.get();
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

	// Redirect
	if (selectedLocation->getRedirectionCode() != -1 && !selectedLocation->getRedirectionsPath().empty()) {
		params.isRedirect = true;
		params.redirectCode = selectedLocation->getRedirectionCode();
		params.redirectUrl = selectedLocation->getRedirectionsPath();
		return params;
	}

	// Methods Allowed
	std::vector<std::string> methods = selectedLocation->getAllowedMethods();
	bool findAllowedMethod = false;
	for (int i = 0; i < (int)methods.size(); i++){
		if (methods[i] == request.getMethod()){
			findAllowedMethod = true;
			break;
		}
	}
	if (!findAllowedMethod){
		params.errorCode = 405;
		return params;
	}
	
	// Max body limit - now checked earlier in request processing
	// if (request.getBody().length() > (size_t)selectedServer->getMaxBodySize() && selectedServer->getMaxBodySize() != -1)
	// {
	// 	params.errorCode = 413;
	// 	return params;
	// }

	std::string fileSystemPath = selectedLocation->getRoot();
	std::string relativePath = request.getPath().substr(selectedLocation->getPath().length());
	if (!relativePath.empty() && relativePath[0] == '/') {
		relativePath = relativePath.substr(1);
	}
	if (!fileSystemPath.empty() && fileSystemPath.back() != '/' && !relativePath.empty() && relativePath[0] != '/')
	{
		fileSystemPath += "/";
	}
	fileSystemPath += relativePath;
	if (fileSystemPath.empty() || fileSystemPath.back() == '/') {
		if (!selectedLocation->getIndex().empty()) {
			std::string indexPath = fileSystemPath;
			if (indexPath.empty() || indexPath.back() != '/') indexPath += "/";
			indexPath += selectedLocation->getIndex();
			if (isFileExists(indexPath)) {
				fileSystemPath = indexPath;
			}
		}
	}
	bool isDir = isDirectory(fileSystemPath);

	// POST
	if (request.getMethod() == "POST") {
		if (!selectedLocation->getUploadPath().empty()) {
			std::string uploadPath = selectedLocation->getUploadPath();
			
			if (!isDirectory(uploadPath) || !hasWriteAccess(uploadPath)) {
				params.errorCode = 500;
				return params;
			}
		
							// Check if this is a CGI script for uploads
		std::string cgiPath = selectedLocation->getRoot() + "/" + relativePath;
		if (cgiPath.find(".py") != std::string::npos) {
			params.isCGI = true;
			params.cgiScriptPath = cgiPath;
			params.cgiTargetFile = fileSystemPath;
			params.uploadTargetDir = uploadPath;
			return params;
		}
			
			params.isUpload = true;
			params.uploadTargetDir = uploadPath;
			std::string relativePath = request.getPath().substr(selectedLocation->getPath().length());
	if (!relativePath.empty() && relativePath[0] == '/') {
		relativePath = relativePath.substr(1);
	}
			params.uploadFilename = relativePath;
			return params;
		}
	}

	// DELETE
	if (request.getMethod() == "DELETE"){
		std::string cgiPath = selectedLocation->getRoot() + "/" + relativePath;
		if (cgiPath.find(".py") != std::string::npos) {
			params.isCGI = true;
			params.cgiScriptPath = cgiPath; // Path to the CGI script
			params.cgiTargetFile = fileSystemPath; // Path to the file being processed by CGI
			if (!selectedLocation->getUploadPath().empty()) {
				params.uploadTargetDir = selectedLocation->getUploadPath();
			} else {
				params.uploadTargetDir = "www/upload";
			}
			return params;
		}

		if (!isFileExists(fileSystemPath) && !isDir) {
			params.errorCode = 404;
		} else if (!isDir) {
			params.filePath = fileSystemPath;
			params.isDeleteOperation = true;
		} else {
			if (request.getPath().back() != '/') { // URI does not end with '/'
				params.errorCode = 409;
			} else {
				if (!hasWriteAccess(fileSystemPath)) {
					params.errorCode = 403;
				} else {
					params.filePath = fileSystemPath;
					params.isDeleteOperation = true;
					params.isDeleteDirectory = true;
				}
			}
		}
		return params;
	}

	//GET
	if (request.getMethod() == "GET"){
		if (isDir){
			if (request.getPath().back() != '/'){
				params.errorCode = 301;
				params.isRedirect = true;
				params.redirectCode = 301;
				params.redirectUrl = request.getPath()+ "/";
			}
			else { // URI ends with '/'
				std::string indexPath;
				if (!selectedLocation->getIndex().empty()) {
					indexPath = fileSystemPath;
					if (indexPath.back() != '/') indexPath += "/";
					indexPath += selectedLocation->getIndex();
				}

				// First, check if the index file exists
				if (!indexPath.empty() && isFileExists(indexPath)) {
					params.isStaticFile = true;
					params.filePath = indexPath;
				}
				// If no index file found, then check for autoindex
				else if(selectedLocation->getAutoindex()){
					params.isStaticFile = true;
					params.isAutoindex = true;
					params.filePath = fileSystemPath;
				}
				else {
					params.errorCode = 403;
				}
			}
		} else if(isFileExists(fileSystemPath)){
			std::cout << "in file exists" << std::endl;
			params.isStaticFile = true;
			params.filePath = fileSystemPath;
		} else{
			params.errorCode = 404;
		}
		return params;
	}
	//CGI
	std::string cgiPath = selectedLocation->getRoot() + "/" + relativePath;
	std::cout << "cgiPath: " << cgiPath << std::endl;
	if (request.getMethod() == "POST" || request.getMethod() == "GET") {
		// if (!isExecutable(cgiPath)) {
		// 	params.errorCode = 500; // Internal Server Error
		// 	return params;
		// }
		params.isCGI = true;
		params.cgiScriptPath = cgiPath; // Path to the CGI script
		params.cgiTargetFile = fileSystemPath; // Path to the file being processed by CGI
		return params;
	}
	params.errorCode = 501;
	return params;
}


bool Router::isFileExists(const std::string& path) const
{
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
}
bool Router::isDirectory(const std::string& path) const
{
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode));
}

bool Router::hasWriteAccess(const std::string& path) const {
	return access(path.c_str(), W_OK) == 0;
}

bool Router::isExecutable(const std::string& path) const {
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0 && (buffer.st_mode & S_IXUSR || buffer.st_mode & S_IXGRP || buffer.st_mode & S_IXOTH));
}