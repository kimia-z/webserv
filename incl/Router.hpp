#ifndef ROUTER_HPP
# define ROUTER_HPP

#include "Webserv42.hpp"

struct ActionParameters {
	const SingleServer*	matchedServer; // Pointer to the determined server config
	const Location*		matchedLocation;   // Pointer to the determined location config

	bool				isRedirect;
	std::string			redirectUrl;
	int					redirectCode;

	bool				isCGI;
	std::string			cgiScriptPath; // e.g., "/usr/bin/php-cgi" (from config)
	std::string			cgiTargetFile; // The actual script file to execute (resolved from URL)

	bool				isUpload;
	std::string			uploadTargetDir; // Directory to save uploaded files (from config)
	std::string			uploadFilename;  // Name of the file being uploaded (from request)

	bool				isStaticFile;
	std::string			filePath;      // Full path to the file on the server's filesystem
	bool				isAutoindex;   // True if directory listing should be generated

	// Error handling: If routing itself determines an error
	int					errorCode;     // e.g., 400, 404, 405, 413
	std::string			errorPagePath; // Path to custom error page (if configured for matchedServer)

	// Default constructor to initialize flags
	ActionParameters() : matchedServer(nullptr), matchedLocation(nullptr),
						 isRedirect(false), redirectCode(0),
						 isCGI(false), isUpload(false), isStaticFile(false),
						 isAutoindex(false), errorCode(0) {}
};

class Router
{

private:
	const Server42& allServers_;
	const SingleServer* selectServerBlock(const Request& request, int listeningPort) const;
	const Location* findBestMatchingLocation(const Request& request, const SingleServer* server) const;
	ActionParameters determineAction(const Request& request, const SingleServer* selectedServer, const Location* selectedLocation) const;
	bool fileExists(const std::string& path) const;
	bool isDirectory(const std::string& path) const;

public:
	Router(const Server42& allServersConfig);
	ActionParameters routeRequest(const Request& request, int listeningPort) const;
};



#endif