#ifndef WEBSERV_HPP
# define WEBSERV_HPP

#include "Webserv42.hpp"

class Webserv {
private:
	const Server42&                     _allServersConfig; 

	int                                 _epollFd;
	std::vector<epoll_event>            _events;
	
	// Maps listener FD to its SingleServer config (for new connections)
	std::map<int, const SingleServer*>  _listenerMap; 

	// Maps client FD to its associated SingleServer (for routing)
	std::map<int, const SingleServer*>  _clientToServerMap;

	// Maps client FD to its Request object (for accumulating incoming data)
	std::map<int, Request>              _clientRequests;
	
	// Maps client FD to its pending raw HTTP response string (for sending)
	std::map<int, std::string>          _clientResponses; 

	// --- Router Instance ---
	Router                              _router; // One instance of the Router

	// --- Private Helper Methods (for Epoll Management) ---
	void _addFdToEpoll(int fd, uint32_t events);
	void _removeFdFromEpoll(int fd);
	void _closeClientConnection(int clientFd); // Utility to clean up a client connection

	// --- Private Helper Methods (for Event Handling) ---
	void _handleNewConnection(int listenerFd);
	void _handleClientRead(int clientFd);
	void _handleClientWrite(int clientFd);

	// --- Private Helper Methods (for File System Operations) ---
	// These functions perform the actual file/directory I/O
	std::string _readFileContent(const std::string& path) const;
	std::string _getMimeType(const std::string& filePath) const;
	std::string _generateDirectoryListing(const std::string& directoryPath) const;
	bool        _fileExists(const std::string& path) const; // Checks for regular files
	bool        _isDirectory(const std::string& path) const; // Checks for directories
	bool        _hasWriteAccess(const std::string& path) const; // Checks write permissions
	bool        _isExecutable(const std::string& path) const; // Checks execute permissions

public:
	// Constructor takes the complete parsed configuration
	Webserv(const Server42& config); 
	Webserv(const Webserv& copy) = delete; // Prevent copying this orchestrator
	Webserv& operator=(const Webserv& copy) = delete;
	~Webserv();

	// Main Server Control Methods
	void start();         // Initializes all listeners and starts the event loop
	void runEventLoop();  // Contains the central epoll_wait loop
};

#endif