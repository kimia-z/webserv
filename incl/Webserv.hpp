#ifndef WEBSERV_HPP
# define WEBSERV_HPP

#include "Webserv42.hpp"
class Server42;
class Router;

class Webserv {
private:
	const Server42&						_allServersConfig; 

	int									epollFd_;
	std::vector<epoll_event>			events_;
	
	// Maps listener FD to its SingleServer config (for new connections)
	std::map<int, const SingleServer*>	listenerMap_;

	// Maps client FD to its associated SingleServer (for routing)
	std::map<int, const SingleServer*>	clientToServerMap_;

	// Maps client FD to its Request object (for accumulating incoming data)
	std::map<int, Request>				clientRequests_;
	
	// Maps client FD to its pending raw HTTP response string (for sending)
	std::map<int, std::string>			clientResponses_;

	// --- Router Instance ---
	Router								router_; // One instance of the Router

	// --- Private Helper Methods (for Epoll Management) ---
	void addFdToEpoll(int fd, uint32_t events);
	void removeFdFromEpoll(int fd);
	void closeClientConnection(int clientFd); // Utility to clean up a client connection

	// --- Private Helper Methods (for Event Handling) ---
	void handleNewConnection(int listenerFd);
	void handleClientRead(int clientFd);
	void handleClientWrite(int clientFd);

	// --- Private Helper Methods (for File System Operations) ---
	// These functions perform the actual file/directory I/O
	std::string	readFileContent(const std::string& path) const;
	// std::string	getMimeType(const std::string& filePath) const;
	std::string	generateDirectoryListing(const std::string& directoryPath) const;
	bool		fileExists(const std::string& path) const; // Checks for regular files
	bool		isDirectory(const std::string& path) const; // Checks for directories
	bool		hasWriteAccess(const std::string& path) const; // Checks write permissions
	// bool		isExecutable(const std::string& path) const; // Checks execute permissions

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