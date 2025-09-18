#ifndef WEBSERV_HPP
# define WEBSERV_HPP

#include "Webserv42.hpp"
#include <dirent.h> //for readdir()
#include <chrono> // for the timeout

class Server42;
class Router;

class Webserv {
private:
	const Server42&											allServersConfig_; 

	int														epollFd_;
	std::vector<epoll_event>								events_;
	std::map<int, const SingleServer*>						listenerMap_;		// Maps listener FD to its SingleServer config (for new connections)
	std::map<int, const SingleServer*>						clientToServerMap_;	// Maps client FD to its associated SingleServer (for routing)
	std::map<int, std::shared_ptr<Cgi>>						clientCgiMap_;		// Maps client FD to its Cgi object (for handling CGI requests)
	std::map<int, Request>									clientRequests_;	// Maps client FD to its Request object (for accumulating incoming data)
	std::map<int, std::string>								clientResponses_;	// Maps client FD to its pending raw HTTP response string (for sending)
	std::map<int, std::chrono::steady_clock::time_point>	clientTimeouts_;	// Maps client FD to its last activity timestamp

	// Core runEventLoop methods
	void		runMaintenanceTasks();
	void		handleEpollWaitError();
	void		processEpollEvents(int numEvents);
	void		handleClientSocketEvent(int currentFd, uint32_t currentEvents);

	// Epoll Managment
	void		addFdToEpoll(int fd, uint32_t events);
	void		removeFdFromEpoll(int fd);
	void		closeClientConnection(int clientFd);

	void		handleClientRead(int clientFd);
	bool		isCgiAlreadyRunning(int clientFd);
	bool		isResponsePending(int clientFd);
	bool		isRequestComplete(int clientFd);
	void		handleCompleteRequest(int clientFd);
	void		switchToWriteMode(int clientFd);
	ssize_t		receiveClientData(int clientFd);
	void		processRequestData(int clientFd, ssize_t bytesReceived);
	void		processCompleteRequest(int clientFd, Request& request);
	void		handleCgiAction(int clientFd, const ActionParameters& action, const SingleServer* clientServer);
	void		handleHttpException(int clientFd, const HttpException& e);
	void		handleErrorAction(int clientFd, const ActionParameters& action);
	void		handleRedirectAction(int clientFd, const ActionParameters& action);
	void		handleStaticFileAction(int clientFd, const ActionParameters& action);
	void		handleUnhandledAction(int clientFd);

	// Event Handeling
	void					handleNewConnection(int listenerFd);
	void					handleClientWrite(int clientFd);
	void					handleCgiEvent(int pipeFd, uint32_t events);
	void					handleCgiAction(int pipeFd, uint32_t events, int clientFd, std::shared_ptr<Cgi> cgi);
	int						findClientFdForPipe(int pipeFd);
	std::shared_ptr<Cgi> 	findCgiForClient(int clientFd);

	// File System Operations
	std::string	readFileContent(const std::string& path) const;
	std::string	generateDirectoryListing(const std::string& directoryPath) const;
	bool		fileExists(const std::string& path) const;

	void		cleanupOldRequests();

	// Timeout Management
	void		checkClientTimeouts();
	bool		isClientStillActive(int clientFd);
	void		updateClientTimeout(int clientFd);

	void		sendTimeoutResponse(int clientFd, int errorCode);
	void		handleTimeout(int clientFd, int timeoutSeconds);
	bool		isTimedOut(const std::chrono::steady_clock::time_point& startTime, const std::chrono::steady_clock::time_point& now, int timeoutSeconds);
	// CGI Helper
	void		checkCgiTimeouts();
	void		checkCompletedCgis();
	void		createCgiResponse(int clientFd, std::shared_ptr<Cgi> cgi);
	void		createCgiErrorResponse(int clientFd, int statusCode, const std::string& message);
	Response	parseCgiOutput(const std::string& output, int statusCode);
	void		parseCgiHeaders(Response& response, const std::string& headers);

public:
	Webserv(const Server42& config); 
	// Webserv(const Webserv& copy);
	// Webserv& operator=(const Webserv& copy) = delete;
	~Webserv();

	// Main Methods
	void		start();		// Initializes all listeners and starts the event loop
	void		runEventLoop();	// Contains the central epoll_wait loop
};

#endif