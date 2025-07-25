#include "../incl/Router.hpp"

Router::Router(const Server42& allServersConfig): allServers_(allServersConfig)
{}
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
	for(int i = 0; i < (int)locations.size(); i++){
		const Location &location = locations[i];
		const std::string &locationPath = location.getPath();
		if (requestPath.find(locationPath, 0) == 0) {
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

	std::string fileSystemPath = selectedLocation->getRoot();
	std::string relativePath = request.getPath().substr(selectedLocation->getPath().length());

	if (!fileSystemPath.empty() && fileSystemPath.back() != '/' && !relativePath.empty() && relativePath[0] != '/')
	{
		fileSystemPath += "/";
	}
	fileSystemPath += relativePath;
		//TODO:double check?
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
	// bool isFile = isFileExists(fileSystemPath);

	// if (selectedLocation->isCGIEnabled() && (request.getMethod() == "GET" || request.getMethod() == "POST")) {
		
	// 	// Extract file extension from the determined filesystemPath
	// 	size_t dotPos = fileSystemPath.rfind('.');
	// 	if (dotPos != std::string::npos) {
	// 		std::string extension = fileSystemPath.substr(dotPos); // Includes the dot, e.g., ".php"

	// 		// Check if this extension is in the list of CGI-allowed extensions for this location
	// 		const std::vector<std::string>& allowedCgiExtensions = selectedLocation->getCGIAllowedExtensions();
	// 		bool extensionMatchesCGI = false;
	// 		for (size_t i = 0; i < allowedCgiExtensions.size(); ++i) {
	// 			if (allowedCgiExtensions[i] == extension) {
	// 				extensionMatchesCGI = true;
	// 				break;
	// 			}
	// 		}

	// 		if (extensionMatchesCGI) {
	// 			// Now, further check if the CGI script file actually exists and is executable
	// 			// You'll need an _isExecutable helper similar to _fileExists / _isDirectory
	// 			if (isFileExists(fileSystemPath) && isExecutable(fileSystemPath)) { // _isExecutable checks permission for execute bit
	// 				params.isCGI = true;
	// 				params.cgiTargetFile = fileSystemPath;
	// 				params.cgiExecutor = selectedLocation->getCGIExecutor(); // Get the interpreter path from config
	// 				return params; // Found CGI, return immediately
	// 			} else {
	// 				// Script not found or not executable, even if extension matches config
	// 				params.errorCode = 404; // Not Found
	// 				// Or 403 Forbidden if it exists but not executable
	// 				return params;
	// 			}
	// 		}
	// 	}
	// }


	// POST
	if (request.getMethod() == "POST" && !selectedLocation->getUploadPath().empty())
	{
		params.isUpload = true;
		params.uploadTargetDir = selectedLocation->getUploadPath();
		// TODO: how is getUploadPath() return the value?
		size_t lastSlash = fileSystemPath.rfind('/');
		if (lastSlash != std::string::npos){
			params.uploadFilename = fileSystemPath.substr(lastSlash + 1);
		} else {
			params.uploadFilename = fileSystemPath;
		}
		return params;
	}	// TODO: what should be the proper error code for if it is POST but there isn't upload path?

	// DELETE
	if (request.getMethod() == "DELETE"){
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
			else if(selectedLocation->getAutoindex()){
				params.isStaticFile = true;
				params.isAutoindex = true;
				params.filePath = fileSystemPath;
			} else{
				params.errorCode = 403;
			}
		} else if(isFileExists(fileSystemPath)){
			params.isStaticFile = true;
			params.filePath = fileSystemPath;
		} else{
			params.errorCode = 404;
		}
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