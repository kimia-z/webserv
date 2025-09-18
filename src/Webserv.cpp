#include "../incl/Webserv.hpp"
#include <ctime>
#include <sstream>
extern volatile sig_atomic_t g_running;

Webserv::Webserv(const Server42& config)
	: allServersConfig_(config),
	epollFd_(-1)
{
	epollFd_ = epoll_create1(0);
	if (epollFd_ == -1) {
		throw std::runtime_error("Webserv: Failed to create epoll instance");
	}
	events_.resize(MAX_EVENTS);
	std::cout << "Webserv created with epoll FD: " << epollFd_ << std::endl;
}

Webserv::~Webserv()
{
	if (epollFd_ != -1) {
		close(epollFd_);
	}
	for (std::map<int, Request>::iterator it = clientRequests_.begin(); it != clientRequests_.end(); ++it) {
		close(it->first);
	}
	clientRequests_.clear();
	clientResponses_.clear();
	clientTimeouts_.clear();
	listenerMap_.clear();
	clientToServerMap_.clear();
	std::cout << "Webserv destructed." << std::endl;
}

void Webserv::start()
{
	std::cout << "Starting Webserv..." << std::endl;
	const std::vector<std::shared_ptr<SingleServer>>& servers = allServersConfig_.getServers();
	if (servers.empty()) {
		throw std::runtime_error("No server configurations loaded. Please check your config file.");
	}
	for (const auto& serverPtr : servers) {
		serverPtr->initSocket();
		addFdToEpoll(serverPtr->getServFd(), EPOLLIN);
		listenerMap_[serverPtr->getServFd()] = serverPtr.get();
		std::cout << GREEN << "Server '" << serverPtr->getServName() << "' listening on port "
					<< serverPtr->getServPortInt() << " (FD: " << serverPtr->getServFd() << ")" 
					<< RESET << std::endl;
	}
	runEventLoop();
}

//our main loop, waits for events and calls appropriate handlers
void	Webserv::runEventLoop() {
	while (g_running == 0) {
		runMaintenanceTasks();
		int numEvents = epoll_wait(epollFd_, events_.data(), events_.size(), 5000);
		if (numEvents == -1) {
			handleEpollWaitError();
			continue;
		}
		processEpollEvents(numEvents);
	}
}

//checks timeouts, cleans up finished cgis
void	Webserv::runMaintenanceTasks() {
	checkClientTimeouts();
	checkCgiTimeouts();
	if (!clientCgiMap_.empty()) {
		checkCompletedCgis();
	}
}

//handles errors from epoll_wait & throws an exception with the real error
void	Webserv::handleEpollWaitError() {
	if (errno == EINTR) {
		std::cerr << "epoll_wait was interrupted by a signal." << std::endl;
		return;
	} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
		return;
	} else {
		std::cerr << RED << "epoll_wait() failed: " << strerror(errno) << RESET << std::endl;
		throw std::runtime_error("epoll_wait failed, critical error.");
	}
}

//runs through all the events and checks if they are listener, cgi pipe or client socket and calls the appropriate handler
void	Webserv::processEpollEvents(int numEvents) {
	for (int i = 0; i < numEvents; i++) {
		int	currentFd = events_[i].data.fd;
		uint32_t currentEvents = events_[i].events;

		std::cout << "DEBUG: Handling event for FD " << currentFd << std::endl;
		std::cout << "DEBUG: Event on FD " << currentFd << " with events " << currentEvents << std::endl;

		if (listenerMap_.count(currentFd)) {
			std::cout << "DEBUG: New connection on listener FD " << currentFd << std::endl;
			handleNewConnection(currentFd);
		} else {
			bool isCgiPipe = false;
			for (auto it = clientCgiMap_.begin(); it != clientCgiMap_.end(); ++it) {
				if (it->second && (it->second->getInputPipe() == currentFd || it->second->getOutputPipe() == currentFd)) {
					std::cout << "DEBUG: CGI pipe event on FD " << currentFd << std::endl;
					handleCgiEvent(currentFd, currentEvents);
					isCgiPipe = true;
					break;
				}
			}
			if (!isCgiPipe) {
				handleClientSocketEvent(currentFd, currentEvents);
			}
		}
	}
}

//handles events on client sockets, checks connection errors, read and write events, closes connections on errors or when done
void	Webserv::handleClientSocketEvent(int currentFd, uint32_t currentEvents) {
	std::cout << "DEBUG: Client socket event on FD " << currentFd << std::endl;
	if (currentEvents & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
		if (clientCgiMap_.find(currentFd) != clientCgiMap_.end()) {
			return; //ignore if cgi pipe
		}
		std::cerr << RED << "Epoll error, hangup or remote shutdown on FD " << currentFd << RESET << std::endl;
		closeClientConnection(currentFd);
	}
	if (currentEvents & EPOLLIN) {
		handleClientRead(currentFd);
	}
	if (currentEvents & EPOLLOUT) {
		handleClientWrite(currentFd);
	}
}

void Webserv::addFdToEpoll(int fd, uint32_t events)
{
	epoll_event event;
	event.events = events | EPOLLRDHUP | EPOLLET;
	event.data.fd = fd;
	if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) == -1) {
		throw std::runtime_error("Failed to add FD to epoll");
	}
}

void Webserv::removeFdFromEpoll(int fd)
{
	if (epollFd_ < 0 || fd < 0) {
		return;
	}

	std::cout << "DEBUG: removeFdFromEpoll called for FD " << fd << std::endl;
	std::cout << "DEBUG: clientRequests_.find(fd) == clientRequests_.end(): " << (clientRequests_.find(fd) == clientRequests_.end()) << std::endl;
	std::cout << "DEBUG: listenerMap_.find(fd) == listenerMap_.end(): " << (listenerMap_.find(fd) == listenerMap_.end()) << std::endl;


	if (clientRequests_.find(fd) == clientRequests_.end() && listenerMap_.find(fd) == listenerMap_.end()) {
		std::cout << "DEBUG: FD " << fd << " not found in maps, returning" << std::endl;
		return;
	}
	std::cout << "DEBUG: Attempting to remove FD " << fd << " from epoll" << std::endl;
	if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, NULL) == -1) {
		std::cout << "DEBUG: epoll_ctl failed with errno: " << errno << " (" << strerror(errno) << ")" << std::endl;
		if (errno != EBADF) {
			std::cerr << RED << "epoll_ctl(DEL, FD " << fd << ") failed: " << strerror(errno) << RESET << std::endl;
		} else {
			std::cout << "DEBUG: Ignoring EBADF error for FD " << fd << std::endl;
		}
	} else {
		std::cout << "DEBUG: Successfully removed FD " << fd << " from epoll" << std::endl;
	}
}

void Webserv::closeClientConnection(int clientFd)
{
	//extra check to prevent double close
	if (clientRequests_.find(clientFd) == clientRequests_.end()) {
		return;
	}
	if (clientCgiMap_.find(clientFd) != clientCgiMap_.end()) {
		try {
			clientCgiMap_.erase(clientFd);
		} catch (const std::exception& e) {
		}
	}
	try {
		removeFdFromEpoll(clientFd);
	} catch (const std::exception& e) {
	}
		
	if (clientFd > 0) {
		close(clientFd);
	}

	clientRequests_.erase(clientFd);	// Remove client's request state
	clientResponses_.erase(clientFd);	// Remove client's response state
	clientToServerMap_.erase(clientFd);	// Remove mapping to server config

	std::cout << YELLOW << "Closed connection for FD: " << clientFd << RESET << std::endl;
}

void Webserv::handleNewConnection(int listenerFd)
{
	struct sockaddr_storage client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	int clientFd = accept(listenerFd, (struct sockaddr *)&client_addr, &client_addr_len);
	if (clientFd == -1) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			std::cerr << RED << "Accept failed on listener FD " << listenerFd << ": " << strerror(errno) << RESET << std::endl;
		}
		return;
	}

	clientToServerMap_[clientFd] = listenerMap_[listenerFd];
	clientRequests_.insert(std::make_pair(clientFd, Request()));
	clientRequests_[clientFd].setMaxBodySize(listenerMap_[listenerFd]->getMaxBodySize());
	updateClientTimeout(clientFd);
	addFdToEpoll(clientFd, EPOLLIN);
	std::cout << GREEN << "Accepted new client (FD: " << clientFd << ") on listener " << listenerFd << RESET << std::endl;
}

// Handles reading data from a client socket
void	Webserv::handleClientRead(int clientFd) {
	if (isCgiAlreadyRunning(clientFd)) {
		std::cout << "DEBUG: CGI already running for client FD " << clientFd << ", ignoring EPOLLIN" << std::endl;
		return;
	}
	if (isResponsePending(clientFd)) {
		std::cout << "DEBUG: Response already pending for client FD " << clientFd << ", ignoring EPOLLIN" << std::endl;
		return;
	}
	if (isRequestComplete(clientFd)) {
		std::cout << "DEBUG: Request already complete for client FD " << clientFd << ", ignoring EPOLLIN" << std::endl;
		handleCompleteRequest(clientFd);
		return;
	}

	ssize_t bytesReceived = receiveClientData(clientFd);
	if (bytesReceived <= 0) {
		return; // Error or disconnect handled in receiveClientData
	}

	processRequestData(clientFd, bytesReceived);
}

//checks if a cgi is already running for this client, if so ignores EPOLLIN events
bool	Webserv::isCgiAlreadyRunning(int clientFd) {
	if (clientCgiMap_.find(clientFd) != clientCgiMap_.end()) {
		std::cout << "DEBUG: CGI already running for client FD " << clientFd << ", ignoring EPOLLIN" << std::endl;
		return (true);
	}
	return (false);
}

//checks if the request is already complete, if so ignores EPOLLIN events
bool	Webserv::isResponsePending(int clientFd) {
	if (clientResponses_.find(clientFd) != clientResponses_.end()) {
		std::cout << "DEBUG: Response already pending for client FD " << clientFd << ", ignoring EPOLLIN" << std::endl;
		return (true);
	}
	return (false);
}

//checks if the request is already complete, if so ignores EPOLLIN events
bool	Webserv::isRequestComplete(int clientFd) {
	std::map<int, Request>::iterator request_it = clientRequests_.find(clientFd);
	if (request_it == clientRequests_.end()) {
		std::cerr << RED << "Error: No Request object found for FD " << clientFd << " during read. Closing connection." << RESET << std::endl;
		closeClientConnection(clientFd);
		return (true);
	}
	return (false);
}

//checks for cgi, turns to write mode (EPOLLOUT) or closes the connection if no cgi and no response is pending
void	Webserv::handleCompleteRequest(int clientFd) {
	if (clientCgiMap_.find(clientFd) != clientCgiMap_.end()) {
		return; //ignore if cgi already running
	}
	if (clientResponses_.find(clientFd) != clientResponses_.end()) {
		switchToWriteMode(clientFd);
	} else {
		closeClientConnection(clientFd);
	}
}

//switches EPOLLIN to EPOLLOUT, prepares to send the response, closes the connection on switch error
void	Webserv::switchToWriteMode(int clientFd) {
	epoll_event event_mod;
	event_mod.events = EPOLLOUT | EPOLLRDHUP | EPOLLET;
	event_mod.data.fd = clientFd;
	if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &event_mod) == -1) {
		closeClientConnection(clientFd);
	}
}

//reads data using recv, handles errors and disconnections
ssize_t	Webserv::receiveClientData(int clientFd) {
	char	buffer[BUFFER_SIZE];
	memset(buffer, 0, sizeof(buffer));

	ssize_t bytesReceived = recv(clientFd, buffer, sizeof(buffer), 0);
	if (bytesReceived == -1) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) { // Real error (not just no data)
			std::cerr << RED << "Recv failed on FD " << clientFd << ": " << strerror(errno) << RESET << std::endl;
			closeClientConnection(clientFd);
		}
		return (-1);
	} else if (bytesReceived == 0) { // Client disconnected
		std::cout << "Client (FD: " << clientFd << ") disconnected." << std::endl;
		closeClientConnection(clientFd);
		return (0);
	}
	return (bytesReceived);
}

//updates timeout, appends data to the request, processes it, handles complete requests and exceptions
void	Webserv::processRequestData(int clientFd, ssize_t bytesReceived) {
	std::map<int, Request>::iterator request_it = clientRequests_.find(clientFd);
	updateClientTimeout(clientFd);
	char buffer[BUFFER_SIZE];
	request_it->second.appendRawData(buffer, bytesReceived);
	try {
		if (!request_it->second.processRequestData()) {
			// Request not complete yet, need more data
			return;
		}
		processCompleteRequest(clientFd, request_it->second);
	} catch (const HttpException& e) {
		handleHttpException(clientFd, e);
	}
}

//processes a complete request, routes it and calls the appropriate handler based on the action parameters
void	Webserv::processCompleteRequest(int clientFd, Request& request) {
	const	SingleServer* clientServer = clientToServerMap_[clientFd];
	if (!clientServer) {
		std::cerr << RED << "Error: No server config found for client FD " << clientFd << ". Closing." << RESET << std::endl;
		closeClientConnection(clientFd);
		return;
	}
	Router	router(allServersConfig_);
	ActionParameters action = router.routeRequest(request, clientServer->getServPortInt());
	if (action.errorCode != 0) {
		handleErrorAction(clientFd, action);
	} else if (action.isCGI || action.isUpload || action.isDeleteOperation) {
		handleCgiAction(clientFd, action, clientServer);
	} else if (action.isRedirect) {
		handleRedirectAction(clientFd, action);
	} else if (action.isStaticFile) {
		handleStaticFileAction(clientFd, action);
	} else {
		handleUnhandledAction(clientFd);
	}
}

//creates and starts a cgi process, removes the client socket from epoll monitoring
void	Webserv::handleCgiAction(int clientFd, const ActionParameters& action, const SingleServer* clientServer) {
	std::map<int, Request>::iterator request_it = clientRequests_.find(clientFd);
	if (request_it == clientRequests_.end()) {
		std::cerr << RED << "Error: No Request object found for FD " << clientFd << " during CGI handling. Closing connection." << RESET << std::endl;
		closeClientConnection(clientFd);
		return;
	}
	std::shared_ptr<Cgi> cgi = clientCgiMap_[clientFd];
	if (!cgi) {
		clientCgiMap_[clientFd] = std::make_shared<Cgi>(request_it->second, action.cgiScriptPath, 
			clientServer->getServName(), clientServer->getServPortInt());
		cgi = clientCgiMap_[clientFd];
	}
	if (!action.uploadTargetDir.empty()) {
		cgi->setUploadDir(action.uploadTargetDir);
	}
	cgi->setMaxFileSize(clientServer->getMaxBodySize());
	cgi->setScriptPath(action.cgiScriptPath);
	cgi->startCgi([this](int fd, uint32_t events) {
		addFdToEpoll(fd, events);
	});
	std::cout << GREEN << "Started CGI for client FD " << clientFd << " with script: " << action.cgiScriptPath << RESET << std::endl;
	std::cout << "DEBUG: CGI Pipes - Input FD: " << cgi->getInputPipe() << ", Output FD: " << cgi->getOutputPipe() << std::endl;
	std::cout << "DEBUG: removing client FD " << clientFd << " from epoll monitoring" << std::endl;

	removeFdFromEpoll(clientFd);
	request_it->second.markComplete();
}

//builds and queues an error response based on the action parameters
void	Webserv::handleHttpException(int clientFd, const HttpException& e) {
	std::cerr << RED << "HTTP Exception: " << e.what() << RESET << std::endl;

	std::map<int, Request>::iterator request_it = clientRequests_.find(clientFd);
	if (request_it == clientRequests_.end()) {
		std::cerr << RED << "Error: No Request object found for FD " << clientFd << " during HTTP exception handling. Closing connection." << RESET << std::endl;
		closeClientConnection(clientFd);
		return;
	}

	Response errorResponse;
	errorResponse.buildErrorResponse(e.getCode(), "");

	request_it->second.markComplete();

	std::string errorResponseString = errorResponse.toString();
	clientResponses_[clientFd] = errorResponseString;

	switchToWriteMode(clientFd);
	request_it->second.clearParsedRequest();
}

void	Webserv::handleErrorAction(int clientFd, const ActionParameters& action) {
	int finalStatusCode = action.errorCode;
	std::string responseContent = action.errorPagePath;
	if (responseContent.empty()) {
		responseContent = "<h1>Error " + std::to_string(finalStatusCode) + "</h1><p>The requested resource could not be processed.</p>";
	} else {
		responseContent = readFileContent(responseContent);
	}

	Response response_object;
	response_object.buildFromAction(action, responseContent, finalStatusCode); 
	std::string rawResponseString = response_object.toString();
	clientResponses_[clientFd] = rawResponseString;

	switchToWriteMode(clientFd);
}

void	Webserv::handleRedirectAction(int clientFd, const ActionParameters& action) {
	int finalStatusCode = action.redirectCode;
	std::string responseContent = action.redirectUrl; 

	Response response_object;
	response_object.buildFromAction(action, responseContent, finalStatusCode); 
	std::string rawResponseString = response_object.toString();
	clientResponses_[clientFd] = rawResponseString;

	switchToWriteMode(clientFd);
}

void	Webserv::handleStaticFileAction(int clientFd, const ActionParameters& action) {
	std::string responseContent = "";
	int finalStatusCode = 200;

	if (action.isAutoindex) {
		responseContent = generateDirectoryListing(action.filePath);
		finalStatusCode = 200;
	} else {
		responseContent = readFileContent(action.filePath);
		if (responseContent.empty() && fileExists(action.filePath)) {
			finalStatusCode = 500;
			responseContent = "<h1>500 Internal Server Error</h1><p>Failed to read static file: " + action.filePath + "</p>";
		} else {
			finalStatusCode = 200;
		}
	}

	Response response_object;
	response_object.buildFromAction(action, responseContent, finalStatusCode); 
	std::string rawResponseString = response_object.toString();
	clientResponses_[clientFd] = rawResponseString;

	switchToWriteMode(clientFd);
}

void	Webserv::handleUnhandledAction(int clientFd) {
	int finalStatusCode = 500;
	std::string responseContent = "<h1>" + std::to_string(finalStatusCode) + " Internal Server Error</h1><p>Unhandled action type after routing.</p>";

	Response response_object;
	response_object.buildFromAction(ActionParameters(), responseContent, finalStatusCode); 
	std::string rawResponseString = response_object.toString();
	clientResponses_[clientFd] = rawResponseString;

	switchToWriteMode(clientFd);
}

// Handles sending data to a client socket
void Webserv::handleClientWrite(int clientFd)
{
	std::map<int, std::string>::iterator response_it = clientResponses_.find(clientFd);
	if (response_it == clientResponses_.end()) {
		closeClientConnection(clientFd);
		return;
	}
	if (response_it->second.empty()) {
		closeClientConnection(clientFd);
		return;
	}

	ssize_t bytesSent = send(clientFd, response_it->second.c_str(), response_it->second.length(), 0);
	if (bytesSent == -1) {
		std::cerr << RED << "Send failed on FD " << clientFd << ": " << strerror(errno) << RESET << std::endl;
		closeClientConnection(clientFd);
	} else {
		updateClientTimeout(clientFd);
		response_it->second.erase(0, bytesSent);

		if (response_it->second.empty()) {
			// All data sent.
			closeClientConnection(clientFd);
			// clientResponses_.erase(clientFd);
		}
	}
}

std::string Webserv::readFileContent(const std::string& path) const
{
	std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		std::cerr << RED << "Error: Could not open file for reading: " << path << RESET << std::endl;
		return "";
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	file.close();
	return buffer.str();
}

bool Webserv::fileExists(const std::string& path) const
{
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
}

std::string Webserv::generateDirectoryListing(const std::string& directoryPath) const
{
	std::stringstream html_listing;
	html_listing << "<!DOCTYPE html>\r\n"
				<< "<html lang=\"en\">\r\n"
				<< "<head>\r\n"
				<< "	<meta charset=\"UTF-8\">\r\n"
				<< "	<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\r\n"
				<< "	<title>Directory Listing</title>\r\n"
				<< "	<style>\r\n"
				<< "		@import url('https://fonts.googleapis.com/css2?family=Poppins:wght@400;600;700&display=swap');\r\n"
				<< "		\r\n"
				<< "		:root {\r\n"
				<< "				--primary-color: #2c3e50;\r\n"
				<< "				--secondary-color: #3498db;\r\n"
				<< "				--accent-color: #e74c3c;\r\n"
				<< "				--text-color: #f4f4f4;\r\n"
				<< "				--bg-color: #ecf0f1;\r\n"
				<< "				--card-bg: #ffffff;\r\n"
				<< "				--shadow: 0 8px 16px rgba(0, 0, 0, 0.1);\r\n"
				<< "		}\r\n"
				<< "\r\n"
				<< "		body {\r\n"
				<< "				font-family: 'Poppins', sans-serif;\r\n"
				<< "				background-color: var(--bg-color);\r\n"
				<< "				margin: 0;\r\n"
				<< "				padding: 2rem;\r\n"
				<< "				color: var(--primary-color);\r\n"
				<< "		}\r\n"
				<< "\r\n"
				<< "		.container {\r\n"
				<< "				background-color: var(--card-bg);\r\n"
				<< "				padding: 2rem 3rem;\r\n"
				<< "				border-radius: 12px;\r\n"
				<< "				box-shadow: var(--shadow);\r\n"
				<< "				max-width: 800px;\r\n"
				<< "				margin: 2rem auto;\r\n"
				<< "		}\r\n"
				<< "\r\n"
				<< "		h1 {\r\n"
				<< "				font-size: 2.5rem;\r\n"
				<< "				font-weight: 700;\r\n"
				<< "				color: var(--primary-color);\r\n"
				<< "				border-bottom: 2px solid #ddd;\r\n"
				<< "				padding-bottom: 1rem;\r\n"
				<< "				margin-top: 0;\r\n"
				<< "		}\r\n"
				<< "\r\n"
				<< "		ul {\r\n"
				<< "				list-style: none;\r\n"
				<< "				padding: 0;\r\n"
				<< "				margin: 0;\r\n"
				<< "		}\r\n"
				<< "\r\n"
				<< "		li {\r\n"
				<< "				display: flex;\r\n"
				<< "				align-items: center;\r\n"
				<< "				padding: 0.8rem 0;\r\n"
				<< "				border-bottom: 1px solid #eee;\r\n"
				<< "		}\r\n"
				<< "\r\n"
				<< "		li:last-child {\r\n"
				<< "				border-bottom: none;\r\n"
				<< "		}\r\n"
				<< "\r\n"
				<< "		a {\r\n"
				<< "				display: flex;\r\n"
				<< "				align-items: center;\r\n"
				<< "				text-decoration: none;\r\n"
				<< "				color: var(--secondary-color);\r\n"
				<< "				font-weight: 600;\r\n"
				<< "				font-size: 1rem;\r\n"
				<< "				transition: color 0.3s ease;\r\n"
				<< "		}\r\n"
				<< "\r\n"
				<< "		a:hover {\r\n"
				<< "				color: var(--accent-color);\r\n"
				<< "		}\r\n"
				<< "\r\n"
				<< "		.icon {\r\n"
				<< "				font-size: 1.2rem;\r\n"
				<< "				margin-right: 0.8rem;\r\n"
				<< "				color: #7f8c8d;\r\n"
				<< "		}\r\n"
				<< "\r\n"
				<< "		.icon-dir::before {\r\n"
				<< "				content: \"📁\";\r\n"
				<< "		}\r\n"
				<< "\r\n"
				<< "		.icon-file::before {\r\n"
				<< "				content: \"📄\";\r\n"
				<< "		}\r\n"
				<< "\r\n"
				<< "		.footer {\r\n"
				<< "				margin-top: 2rem;\r\n"
				<< "				text-align: center;\r\n"
				<< "				font-size: 0.8rem;\r\n"
				<< "				color: #7f8c8d;\r\n"
				<< "		}\r\n"
				<< "	</style>\r\n"
				<< "</head>\r\n"
				<< "<body>\r\n"
				<< "	<div class=\"container\">\r\n"
				<< "		<h1>Directory Listing for " << directoryPath << "</h1>\r\n"
				<< "		<ul>\r\n"
				<< "				<li>\r\n"
				<< "					<a href=\"../\">\r\n"
				<< "						<span class=\"icon icon-dir\"></span>\r\n"
				<< "						.. (Parent Directory)\r\n"
				<< "					</a>\r\n"
				<< "				</li>\r\n";

	DIR *dir = opendir(directoryPath.c_str());
	if (dir == NULL) {
		std::cerr << RED << "Error opening directory for listing: " << directoryPath << ": " << strerror(errno) << RESET << std::endl;
		html_listing << "<p>Error: Could not open directory.</p>\r\n";
	} else {
		struct dirent *entry;
		while ((entry = readdir(dir)) != NULL) {
			std::string name = entry->d_name;
			if (name == "." || name == "..") continue; 

			std::string fullEntryPath = directoryPath;
			if (fullEntryPath.back() != '/') fullEntryPath += "/";
			fullEntryPath += name;
			
			struct stat entry_stat;
			if (stat(fullEntryPath.c_str(), &entry_stat) == 0 && S_ISDIR(entry_stat.st_mode)) {
				html_listing << "				<li>\r\n"
							<< "					<a href=\"" << name << "/\">\r\n"
							<< "						<span class=\"icon icon-dir\"></span>\r\n"
							<< "						" << name << "/\r\n"
							<< "					</a>\r\n"
							<< "				</li>\r\n";
			} else {
				html_listing << "				<li>\r\n"
							<< "					<a href=\"" << name << "\">\r\n"
							<< "						<span class=\"icon icon-file\"></span>\r\n"
							<< "						" << name << "\r\n"
							<< "					</a>\r\n"
							<< "				</li>\r\n";
			}
		}
		closedir(dir);
	}

	html_listing << "		</ul>\r\n"
				<< "	</div>\r\n"
				<< "	<div class=\"footer\">\r\n"
				<< "		Generated by `webserv`.\r\n"
				<< "	</div>\r\n"
				<< "</body>\r\n"
				<< "</html>\r\n";
	return html_listing.str();
}

// //TODO - really necessary?
// void Webserv::cleanupOldRequests(){
// 	auto currentTime = std::chrono::steady_clock::now();
// 	const int CLEANUP_TRESHOLD = 60; // 1 minute
// 	for (auto it = clientTimeouts_.begin(); it != clientTimeouts_.end();) {
// 		if (currentTime - it->second > std::chrono::seconds(CLEANUP_TRESHOLD)) {
// 			int clientFd = it->first;

// 			try {
// 				closeClientConnection(clientFd);
// 			} catch (const std::exception& e) {
// 				std::cerr << "Error cleaning up FD " << clientFd << ": " << e.what() << std::endl;
// 			}
// 			it = clientTimeouts_.erase(it);
// 			std::cout << YELLOW << "Cleaning up old request for FD " << clientFd << " after " << CLEANUP_TRESHOLD << " seconds" << RESET << std::endl;
// 		} else {
// 			++it;
// 		}
// 	}
// }

void	Webserv::checkClientTimeouts() {
	auto now = std::chrono::steady_clock::now();
	const int TIMEOUT_SECONDS = 300; // 5 minutes

	for (auto it = clientTimeouts_.begin(); it != clientTimeouts_.end();) {
		if (isTimedOut(it->second, now, TIMEOUT_SECONDS)) {
			handleTimeout(it->first, TIMEOUT_SECONDS);
			it = clientTimeouts_.erase(it);
		} else {
			++it;
		}
	}
}

void	Webserv::checkCgiTimeouts() {
	auto now = std::chrono::steady_clock::now();
	const int CGI_TIMEOUT_SECONDS = 30; // 30 seconds

	for (auto it = clientCgiMap_.begin(); it != clientCgiMap_.end();) {
		auto startTime = std::chrono::steady_clock::time_point(std::chrono::seconds(it->second->getStartTime()));
		if (isTimedOut(startTime, now, CGI_TIMEOUT_SECONDS)) {
			handleTimeout(it->first, CGI_TIMEOUT_SECONDS);
			it = clientCgiMap_.erase(it);
		} else {
			++it;
		}
	}
}

bool	Webserv::isTimedOut(const std::chrono::steady_clock::time_point& startTime, const std::chrono::steady_clock::time_point& now, int timeoutSeconds)
{
	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
	return (elapsed > timeoutSeconds);
}

void	Webserv::handleTimeout(int clientFd, int timeoutSeconds) {
	std::cout << YELLOW << "CGI for client FD " << clientFd << " timed out after " << timeoutSeconds << " seconds" << RESET << std::endl;
	if (!isClientStillActive(clientFd)) {
		return;
	}
	try {
		if (timeoutSeconds == 300)
			sendTimeoutResponse(clientFd, 408);
		else
			sendTimeoutResponse(clientFd, 504);
		switchToWriteMode(clientFd);
	} catch (const std::exception& e) {
		std::cerr << "Error handling CGI timeout for FD " << clientFd << ": " << e.what() << std::endl;
		closeClientConnection(clientFd);
	}
}

bool	Webserv::isClientStillActive(int clientFd) {
	if (clientRequests_.find(clientFd) == clientRequests_.end()) {
		std::cout << YELLOW << "Client FD " << clientFd << " already disconnected." << RESET << std::endl;
		return (false);
	}
	return (true);
}

void Webserv::updateClientTimeout(int clientFd)
{
	clientTimeouts_[clientFd] = std::chrono::steady_clock::now();
}

void Webserv::sendTimeoutResponse(int clientFd, int errorCode)
{
	std::string errorPagePath;
	if (errorCode == 408) {
		errorPagePath = "error/408.html";
	}
	else {
		errorPagePath = "error/504.html";
	}
	std::string errorPageContent;
		
	if (fileExists(errorPagePath)) {
		errorPageContent = readFileContent(errorPagePath);
	} else {
		errorPageContent = "<html><head><title>" + std::to_string(errorCode) + " Error</title></head>"
							"<body><h1>" + std::to_string(errorCode) + " Error</h1>"
							"<p>Your request took too long to complete.</p>"
							"<p><a href=\"/\">Return to homepage</a></p></body></html>";
	}
	Response errorResponse;
	errorResponse.setStatusCode(errorCode);
	errorResponse.setHeader("Content-Type", "text/html");
	errorResponse.setHeader("Connection", "close");
	errorResponse.setBody(errorPageContent);

	clientResponses_[clientFd] = errorResponse.toString();
}

void Webserv::checkCompletedCgis()
{
	for (auto it = clientCgiMap_.begin(); it != clientCgiMap_.end();) {
		if (it->second && it->second->getIsCgiComplete() && it->second->getIsOutputComplete() && it->second->getIsInputComplete()) {
			it = clientCgiMap_.erase(it);
		} else {
			++it;
		}
	}
}

void	Webserv::handleCgiEvent(int pipeFd, uint32_t events) {
	std::cout << "DEBUG: handleCgiEvent called for FD " << pipeFd << " with events " << events << std::endl;

	int clientFd = findClientFdForPipe(pipeFd);
	if (clientFd == -1) {
		std::cout << "DEBUG: No client found for pipe FD " << pipeFd << std::endl;
		return;
	}

	std::shared_ptr<Cgi> cgi = findCgiForClient(clientFd);
	if (!cgi) {
		std::cout << "DEBUG: No CGI object found for client FD " << clientFd << std::endl;
		return;
	}

	handleCgiAction(pipeFd, events, clientFd, cgi);
}

void	Webserv::handleCgiAction(int pipeFd, uint32_t events, int clientFd, std::shared_ptr<Cgi> cgi) {
	// Write to input pipe
	if (events & EPOLLOUT && pipeFd == cgi->getInputPipe()) {
		std::cout << "DEBUG: EPOLLOUT on input pipe: " << pipeFd << ", calling writeToCgiInput()" << std::endl;
		if (cgi->writeToCgiInput()) {
			std::cout << "DEBUG: Input writing complete, removing pipe: " << pipeFd << " from epoll" << std::endl;
			removeFdFromEpoll(pipeFd);
		} else {
			std::cout << "DEBUG: More data to write, keeping pipe " << pipeFd << " in epoll" << std::endl;
		}
		return;
	}
	// Read from output pipe
	if (events & EPOLLIN && pipeFd == cgi->getOutputPipe()) {
		std::cout << "DEBUG: EPOLLIN on output pipe, calling readFromCgiOutput()" << std::endl;
		cgi->readFromCgiOutput();
	}
	// Handle hang-up on output pipe
	if (events & EPOLLHUP && pipeFd == cgi->getOutputPipe()) {
		std::cout << "DEBUG: EPOLLHUP on output pipe " << pipeFd << std::endl;
		cgi->readFromCgiOutput();
		cgi->markOutputComplete();
		if (!cgi->getIsCgiComplete()) {
			cgi->forceComplete();
		}
		removeFdFromEpoll(pipeFd);
		createCgiResponse(clientFd, cgi);
		return;
	}
	//check if cgi is complete
	if (cgi->getIsCgiComplete() && cgi->getIsOutputComplete() && cgi->getIsInputComplete()) {
		std::cout << "DEBUG: CGI complete, creating response" << std::endl;
		createCgiResponse(clientFd, cgi);
		return;
	}
	//error handling
	if (events & (EPOLLERR | EPOLLRDHUP)) {
		std::cout << "DEBUG: EPOLLERR or EPOLLRDHUP on pipe " << pipeFd << std::endl;
		removeFdFromEpoll(pipeFd);
	}
}


int	Webserv::findClientFdForPipe(int pipeFd) {
	for (auto it = clientCgiMap_.begin(); it != clientCgiMap_.end(); ++it) {
		if (it->second && (it->second->getInputPipe() == pipeFd || it->second->getOutputPipe() == pipeFd)) {
			std::cout << "DEBUG: Found client FD " << it->first << " for pipe FD " << pipeFd << std::endl;
			return (it->first);
		}
	}
	std::cout << "DEBUG: No client found for pipe FD " << pipeFd << std::endl;
	removeFdFromEpoll(pipeFd);
	return (-1);
}

std::shared_ptr<Cgi> Webserv::findCgiForClient(int clientFd) {
	std::shared_ptr<Cgi> cgi = clientCgiMap_[clientFd];
	if (!cgi) {
		std::cout << "DEBUG: No CGI object found for client FD " << clientFd << std::endl;
		removeFdFromEpoll(clientFd);
		return (nullptr);
	}
	return (cgi);
}

void	Webserv::createCgiResponse(int clientFd, std::shared_ptr<Cgi> cgi) {
	std::string	responseContent = cgi->getCgiOutput();
	int	finalStatusCode = cgi->getCgiStatusCode();

	if (responseContent.empty()) {
		createCgiErrorResponse(clientFd, 500, "CGI produced empty output");
		return ;
	}
	Response response = parseCgiOutput(responseContent, finalStatusCode);
	clientResponses_[clientFd] = response.toString();
	switchToWriteMode(clientFd);
	clientCgiMap_.erase(clientFd);
}

void	Webserv::createCgiErrorResponse(int clientFd, int statusCode, const std::string& message) {
	std::string responseContent = "<h1>" + std::to_string(statusCode) + " Internal Server Error</h1><p>" + message + "</p>";
	Response response;
	response.setStatusCode(statusCode);
	response.setHeader("Content-Type", "text/html");
	response.setHeader("Connection", "close");
	response.setBody(responseContent);
	clientResponses_[clientFd] = response.toString();
	switchToWriteMode(clientFd);
	clientCgiMap_.erase(clientFd);
}

Response	Webserv::parseCgiOutput(const std::string& output, int statusCode) {
	Response response;
	response.setStatusCode(statusCode);

	size_t headerEnd = output.find("\n\n");
	if (headerEnd != std::string::npos) {
		parseCgiHeaders(response, output.substr(0, headerEnd));
		response.setBody(output.substr(headerEnd + 2));
	} else {
		response.setBody(output);
	}
	return (response);
}

void	Webserv::parseCgiHeaders(Response& response, const std::string& headers) {
	std::istringstream headerStream(headers);
	std::string line;
	while (std::getline(headerStream, line)) {
		size_t colonPos = line.find(":");
		if (colonPos != std::string::npos) {
			std::string headerName = line.substr(0, colonPos);
			std::string headerValue = line.substr(colonPos + 1);

			headerValue.erase(0, headerValue.find_first_not_of(" \t"));
			headerValue.erase(headerValue.find_last_not_of(" \t\r\n") + 1);
			response.setHeader(headerName, headerValue);
		}
	}
}
