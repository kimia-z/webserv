#ifndef ROUTER_HPP
# define ROUTER_HPP

#include "Webserv42.hpp"
#include <sys/stat.h> 

class Server42;


struct ActionParameters {
	const SingleServer*	matchedServer;
	const Location*		matchedLocation;

	bool				isRedirect;
	std::string			redirectUrl;
	int					redirectCode;

	bool				isCGI;
	std::string			cgiScriptPath;
	std::string			cgiTargetFile;

	bool				isUpload;
	std::string			uploadTargetDir;	// Directory to save uploaded files (from config)
	std::string			uploadFilename;		// Name of the file being uploaded (from request)

	bool				isStaticFile;
	std::string			filePath;
	bool				isAutoindex;

	int					errorCode;
	std::string			errorPagePath;

	bool				isDeleteOperation;
	bool				isDeleteDirectory;

	ActionParameters() : matchedServer(nullptr), matchedLocation(nullptr),
						 isRedirect(false), redirectCode(0),
						 isCGI(false), isUpload(false), isStaticFile(false),
						 isAutoindex(false), errorCode(0), isDeleteOperation(false),
						 isDeleteDirectory(false) {}
};

class Router
{

private:
	const Server42&		allServers_;

	const SingleServer*	selectServerBlock(const Request& request, int listeningPort) const;
	const Location*		findBestMatchingLocation(const Request& request, const SingleServer* server) const;
	ActionParameters	determineAction(const Request& request, const SingleServer* selectedServer, const Location* selectedLocation) const;
	bool				isFileExists(const std::string& path) const;
	bool				isDirectory(const std::string& path) const;
	bool				hasWriteAccess(const std::string& path) const;

public:

	Router(const Server42& allServersConfig);
	~Router();

	// Main Method
	ActionParameters	routeRequest(const Request& request, int listeningPort) const;

	// Helper
	bool				isExecutable(const std::string& path) const;
};



#endif