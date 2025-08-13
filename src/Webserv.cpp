#include "../incl/Webserv.hpp"

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

void Webserv::runEventLoop()
{
	while (true) {
		int numEvents = epoll_wait(epollFd_, events_.data(), events_.size(), -1);
		if (numEvents == -1) {
			throw std::runtime_error("epoll_wait failed, critical error.");
		}
		for (int i = 0; i < numEvents; ++i) {
			int currentFd = events_[i].data.fd;
			uint32_t currentEvents = events_[i].events;

			if (currentEvents & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
				std::cerr << RED << "Epoll error, hangup or remote shutdown on FD " << currentFd << RESET << std::endl;
				closeClientConnection(currentFd);
				continue;
			}

			// If it's a listening socket (new connection)
			if (listenerMap_.count(currentFd)) {
				handleNewConnection(currentFd);
			}
			// If it's a client socket (data to read or write)
			else {
				if (currentEvents & EPOLLIN) {
					handleClientRead(currentFd);
				}
				if (currentEvents & EPOLLOUT) {
					handleClientWrite(currentFd);
				}
			}
		}
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
	if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, NULL) == -1) {
		std::cerr << RED << "epoll_ctl(DEL, FD " << fd << ") failed: " << strerror(errno) << RESET << std::endl;
	}
	// ?????? it should throw or not?!
}

void Webserv::closeClientConnection(int clientFd)
{
	removeFdFromEpoll(clientFd);		// Remove from epoll monitoring
	close(clientFd);					// Close the actual socket FD

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

	// Map this client to the SingleServer config that accepted it (for routing)
	clientToServerMap_[clientFd] = listenerMap_[listenerFd];
	clientRequests_.insert(std::make_pair(clientFd, Request()));
	addFdToEpoll(clientFd, EPOLLIN);
	std::cout << GREEN << "Accepted new client (FD: " << clientFd << ") on listener " << listenerFd << RESET << std::endl;
}

void Webserv::handleClientRead(int clientFd)
{
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, sizeof(buffer));

	std::map<int, Request>::iterator request_it = clientRequests_.find(clientFd);
	if (request_it == clientRequests_.end()) {
		std::cerr << RED << "Error: No Request object found for FD " << clientFd << " during read. Closing connection." << RESET << std::endl;
		closeClientConnection(clientFd);
		return;
	}
	ssize_t bytesReceived = recv(clientFd, buffer, sizeof(buffer), 0);
	if (bytesReceived == -1) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) { // Real error (not just no data)
			std::cerr << RED << "Recv failed on FD " << clientFd << ": " << strerror(errno) << RESET << std::endl;
			closeClientConnection(clientFd);
		}
		return;
	} else if (bytesReceived == 0) { // Client disconnected
		std::cout << "Client (FD: " << clientFd << ") disconnected." << std::endl;
		closeClientConnection(clientFd);
		return;
	} else { // Data received
		request_it->second.appendRawData(buffer, bytesReceived);
		try{
			while (request_it->second.processRequestData())
			{
				const SingleServer* clientServer = clientToServerMap_[clientFd];
				if (!clientServer) {
					std::cerr << RED << "Error: No server config found for client FD " << clientFd << ". Closing." << RESET << std::endl;
					closeClientConnection(clientFd);
					return;
				}
				Router router(allServersConfig_);
				ActionParameters action = router.routeRequest(request_it->second, clientServer->getServPortInt());

				std::string responseContent = "";
				int finalStatusCode = 0;

				// Handle immediate errors from Router
				if (action.errorCode != 0) {
					finalStatusCode = action.errorCode;
					responseContent = action.errorPagePath;
					if (responseContent.empty()) { // If no custom page, provide a generic one
						responseContent = "<h1>Error " + std::to_string(finalStatusCode) + "</h1><p>The requested resource could not be processed.</p>";
					} else {
						responseContent = readFileContent(responseContent);
					}
				}

				else if (action.isCGI || action.isUpload || action.isDeleteOperation) {
					// do something??
					// change the state of Epoll
					// clear request
					// return
				}

				// Usual Response
				else if (action.isRedirect) {
					finalStatusCode = action.redirectCode;
					responseContent = action.redirectUrl; 
				}
				else if (action.isStaticFile) {
					if (action.isAutoindex) {
						responseContent = generateDirectoryListing(action.filePath);
						finalStatusCode = 200;
					} else {
						responseContent = readFileContent(action.filePath);
						if (responseContent.empty() && fileExists(action.filePath)) {
							finalStatusCode = 500; // Read error
							responseContent = "<h1>500 Internal Server Error</h1><p>Failed to read static file: " + action.filePath + "</p>";
						} else {
							finalStatusCode = 200;
						}
					}
				}
				else { // final safety net
					finalStatusCode = action.errorCode;
					if (finalStatusCode == 0) finalStatusCode = 500; // Default to 500 if no specific error
					responseContent = "<h1>" + std::to_string(finalStatusCode) + " Internal Server Error</h1><p>Unhandled action type after routing.</p>";
				}

				// Response Builder
				Response response_object;
				response_object.buildFromAction(action, responseContent, finalStatusCode); 
				std::string rawResponseString = response_object.toString();
				clientResponses_[clientFd] = rawResponseString;

				// Change epoll event to EPOLLOUT to start sending the response
				epoll_event event_mod;
				event_mod.events = EPOLLOUT | EPOLLIN | EPOLLRDHUP | EPOLLET;
				event_mod.data.fd = clientFd;
				if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &event_mod) == -1) {
					closeClientConnection(clientFd);
				}
				request_it->second.clearParsedRequest();
			}
		} catch (const HttpException& e) {
			std::cerr << RED << "HTTP Exception: " << e.what() << RESET << std::endl;
			Response errorResponse;
			errorResponse.setStatusCode(e.getCode());
			errorResponse.setBody(e.getMessage());
			clientResponses_[clientFd] = errorResponse.toString();

			// Switch to EPOLLOUT to send the error response
			epoll_event event_mod;
			event_mod.events = EPOLLOUT | EPOLLIN | EPOLLRDHUP | EPOLLET;
			event_mod.data.fd = clientFd;
			if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &event_mod) == -1) {
				closeClientConnection(clientFd);
			}
			request_it->second.clearParsedRequest();
		}
	}
}

// Handles sending data to a client socket
void Webserv::handleClientWrite(int clientFd)
{
	std::map<int, std::string>::iterator response_it = clientResponses_.find(clientFd);
	if (response_it == clientResponses_.end() || response_it->second.empty()) {
		// No response pending or already sent. Switch back to EPOLLIN.
		epoll_event event_mod;
		event_mod.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
		event_mod.data.fd = clientFd;
		if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &event_mod) == -1) {
			closeClientConnection(clientFd);
		}
		return;
	}

	ssize_t bytesSent = send(clientFd, response_it->second.c_str(), response_it->second.length(), 0);
	if (bytesSent == -1) {
		std::cerr << RED << "Send failed on FD " << clientFd << ": " << strerror(errno) << RESET << std::endl;
		closeClientConnection(clientFd);
	} else {
		// Data sent.
		response_it->second.erase(0, bytesSent);

		if (response_it->second.empty()) {
			// All data sent.
			clientResponses_.erase(clientFd);

			// Switch back to EPOLLIN
			epoll_event event_mod;
			event_mod.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
			event_mod.data.fd = clientFd;
			if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &event_mod) == -1) {
				closeClientConnection(clientFd);
			}
		}
	}
}

// Reads content of a file into a string
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

// Checks if a path points to a regular file
bool Webserv::fileExists(const std::string& path) const
{
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
}

// Checks if a path points to a directory
bool Webserv::isDirectory(const std::string& path) const
{
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode));
}

// Checks if a path has write access
// bool Webserv::hasWriteAccess(const std::string& path) const
// {
// 	return access(path.c_str(), W_OK) == 0;
// }

// Generates an HTML listing for a directory
std::string Webserv::generateDirectoryListing(const std::string& directoryPath) const
{
	std::stringstream html_listing;
	html_listing << "<!DOCTYPE html>\r\n"
				 << "<html>\r\n"
				 << "<head><title>Directory Listing for " << directoryPath << "</title></head>\r\n"
				 << "<body>\r\n"
				 << "<h1>Directory Listing for " << directoryPath << "</h1>\r\n"
				 << "<hr>\r\n"
				 << "<ul>\r\n";

	DIR *dir = opendir(directoryPath.c_str());
	if (dir == NULL) {
		std::cerr << RED << "Error opening directory for listing: " << directoryPath << ": " << strerror(errno) << RESET << std::endl;
		html_listing << "<p>Error: Could not open directory.</p>\r\n";
	} else {
		struct dirent *entry;
		while ((entry = readdir(dir)) != NULL) {
			std::string name = entry->d_name;
			// Skip current directory (.) and parent directory (..)
			if (name == "." || name == "..") continue; 

			std::string fullEntryPath = directoryPath;
			if (fullEntryPath.back() != '/') fullEntryPath += "/"; // Ensure trailing slash if missing
			fullEntryPath += name;

			html_listing << "<li><a href=\""; // Start link
			html_listing << name; // Link target name

			// Append a trailing slash to the link if it's a directory
			// Use stat() to check if it's a directory, as d_type might not be reliable on all systems
			struct stat entry_stat;
			if (stat(fullEntryPath.c_str(), &entry_stat) == 0 && S_ISDIR(entry_stat.st_mode)) {
				html_listing << "/"; 
			}
			html_listing << "\">" << name; // Link text
			if (stat(fullEntryPath.c_str(), &entry_stat) == 0 && S_ISDIR(entry_stat.st_mode)) {
				html_listing << "/"; 
			}
			html_listing << "</a></li>\r\n";
		}
		closedir(dir); // Close the directory stream
	}

	html_listing << "</ul>\r\n"
				 << "<hr>\r\n" // Another horizontal rule
				 << "</body>\r\n"
				 << "</html>\r\n";
	return html_listing.str();
}

