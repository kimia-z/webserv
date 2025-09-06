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

void Webserv::runEventLoop()
{
	while (g_running == 0) {
		checkClientTimeouts();
		checkCgiTimeouts(); // change: CGI timeout (to prevent hanging processes)
		if (!clientCgiMap_.empty()) {
			checkCompletedCgis(); // change: periodic cleanup of completed CGI processes
		}
		int numEvents = epoll_wait(epollFd_, events_.data(), events_.size(), 5000);
		if (numEvents == -1) {
			if (errno == EINTR) {
				std::cerr << "epoll_wait was interrupted by a signal." << std::endl;
				continue;
			} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
				continue;
			} else {
				std::cerr << RED << "epoll_wait() failed: " << strerror(errno) << RESET << std::endl;
				throw std::runtime_error("epoll_wait failed, critical error.");
			}
		}
		for (int i = 0; i < numEvents; ++i) {
			int currentFd = events_[i].data.fd;
			uint32_t currentEvents = events_[i].events;
			if (listenerMap_.count(currentFd)) {
				handleNewConnection(currentFd);
			} else { // Handle as client or CGI pipe
				bool isCgiPipe = false;
				for (auto it = clientCgiMap_.begin(); it != clientCgiMap_.end(); ++it) {
					if (it->second && (it->second->getInputPipe() == currentFd || it->second->getOutputPipe() == currentFd)) {
						handleCgiEvent(currentFd, currentEvents); // change: CGI pipe detection (to prevent race conditions)
						isCgiPipe = true;
						break;
					}
				}
				if (!isCgiPipe) { // If not a CGI pipe, handle as client socket
					if (currentEvents & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
						if (clientCgiMap_.find(currentFd) != clientCgiMap_.end()) {
							continue; // change: error handling (also includes now CGI to prevent client closure before CGI is done)
						}
						std::cerr << RED << "Epoll error, hangup or remote shutdown on FD " << currentFd << RESET << std::endl;
						closeClientConnection(currentFd);
						continue;
					}
					if (currentEvents & EPOLLIN) { handleClientRead(currentFd); }
					if (currentEvents & EPOLLOUT) { handleClientWrite(currentFd); }
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
	if (epollFd_ < 0 || fd < 0) {
		return;
	}
	if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, NULL) == -1) {
		// Only log error if it's not a "Bad file descriptor" error
		if (errno != EBADF) {
			std::cerr << RED << "epoll_ctl(DEL, FD " << fd << ") failed: " << strerror(errno) << RESET << std::endl;
		}
	}
}

void Webserv::closeClientConnection(int clientFd)
{
	// Clean up CGI process first if exists
	if (clientCgiMap_.find(clientFd) != clientCgiMap_.end()) {
		try {
			clientCgiMap_.erase(clientFd);
		} catch (const std::exception& e) {
		}
	}

	// Remove from epoll monitoring
	try {
		removeFdFromEpoll(clientFd);
	} catch (const std::exception& e) {
	}
	
	// Close the actual socket FD
	if (clientFd > 0) {
		close(clientFd);
	}

	// Remove client data from maps
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
	// Set max body size for this request
	clientRequests_[clientFd].setMaxBodySize(listenerMap_[listenerFd]->getMaxBodySize());
	updateClientTimeout(clientFd); // Set initial timeout
	addFdToEpoll(clientFd, EPOLLIN);
	std::cout << GREEN << "Accepted new client (FD: " << clientFd << ") on listener " << listenerFd << RESET << std::endl;
}

void Webserv::handleClientRead(int clientFd)
{
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, sizeof(buffer));

	// Check if client already has a response pending
	if (clientResponses_.find(clientFd) != clientResponses_.end()) {
		return;
	}
	
	std::map<int, Request>::iterator request_it = clientRequests_.find(clientFd);
	if (request_it == clientRequests_.end()) {
		std::cerr << RED << "Error: No Request object found for FD " << clientFd << " during read. Closing connection." << RESET << std::endl;
		closeClientConnection(clientFd);
		return;
	}
	
	// Check if request is already complete
	if (request_it->second.isRequestComplete()) {
		// Request already complete, but we're still getting EPOLLIN events
		// This can happen with large uploads or persistent connections
		
		// Check if this is a CGI request that's still processing
		if (clientCgiMap_.find(clientFd) != clientCgiMap_.end()) {
			// change: wait for CGI to finish before closing client connection (return and wait for CGI to complete)
			return;
		}
		
		// Switch to EPOLLOUT if we have a response to send, otherwise close
		if (clientResponses_.find(clientFd) != clientResponses_.end()) {
			// We have a response to send, switch to write mode
			epoll_event event_mod;
			event_mod.events = EPOLLOUT | EPOLLRDHUP | EPOLLET;
			event_mod.data.fd = clientFd;
			if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &event_mod) == -1) {
				closeClientConnection(clientFd);
			}
		} else {
			// No response pending, close the connection
			closeClientConnection(clientFd);
		}
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
		updateClientTimeout(clientFd); // Update timeout on activity
		request_it->second.appendRawData(buffer, bytesReceived);

		try{
			// Process request data - if it returns false, we need more data
			if (!request_it->second.processRequestData()) {
				// Request not complete yet, need more data
				return;
			}
			
			// Request is complete, process it
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
				// change: CGI is asynchronous = create CGI object and start process TODO
				std::shared_ptr<Cgi>	cgi = clientCgiMap_[clientFd];
				if (!cgi) {
					clientCgiMap_[clientFd] = std::make_shared<Cgi>(request_it->second, action.cgiScriptPath, 
						clientServer->getServName(), clientServer->getServPortInt());
					cgi = clientCgiMap_[clientFd];
				}
				
				// Set upload directory and max file size for CGI
				if (!action.uploadTargetDir.empty()) {
					cgi->setUploadDir(action.uploadTargetDir);
				}
				cgi->setMaxFileSize(clientServer->getMaxBodySize());
				cgi->setScriptPath(action.cgiScriptPath);
				
				// Start CGI process and add pipes to epoll for non-blocking operation
				cgi->startCgi([this](int fd, uint32_t events) {
					addFdToEpoll(fd, events);
				});
				std::cout << GREEN << "Started CGI for client FD " << clientFd << " with script: " << action.cgiScriptPath << RESET << std::endl;
				
				// change: request can be marked as complete - CGI will be handled asynchronously
				request_it->second.markComplete();
				
				// change: client connection stays open for CGI response (client socket will be switched to write mode in handleCgiEvent when CGI finishes)
				return;
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
			event_mod.events = EPOLLOUT | EPOLLRDHUP | EPOLLET;
			event_mod.data.fd = clientFd;
			if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &event_mod) == -1) {
				closeClientConnection(clientFd);
			}
			request_it->second.clearParsedRequest();
		} catch (const HttpException& e) {
			std::cerr << RED << "HTTP Exception: " << e.what() << RESET << std::endl;
			Response errorResponse;
			
			// Mark request as complete to prevent further data reading
			request_it->second.markComplete();

			// Serialize the error response and add it to the clientResponses_ map
				std::string errorResponseString = errorResponse.toString();
				clientResponses_[clientFd] = errorResponseString;

				// Change epoll event to EPOLLOUT to start sending the error response
				epoll_event event_mod;
				event_mod.events = EPOLLOUT | EPOLLRDHUP | EPOLLET;
				event_mod.data.fd = clientFd;
				if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &event_mod) == -1) {
					std::cerr << RED << "Failed to switch FD " << clientFd << " to EPOLLOUT: " << strerror(errno) << RESET << std::endl;
					closeClientConnection(clientFd);
			
			} else {
				// Send the response immediately
				handleClientWrite(clientFd);
			}
			request_it->second.clearParsedRequest();
		}
	}
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
		// Data sent.
		updateClientTimeout(clientFd); // Update timeout on activity
		response_it->second.erase(0, bytesSent);

		if (response_it->second.empty()) {
			// All data sent.
			closeClientConnection(clientFd);
			// clientResponses_.erase(clientFd);
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

void Webserv::checkClientTimeouts()
{
	time_t currentTime = time(NULL);
	const int TIMEOUT_SECONDS = 300;
	
	for (std::map<int, time_t>::iterator it = clientTimeouts_.begin(); it != clientTimeouts_.end();) {
		if (currentTime - it->second > TIMEOUT_SECONDS) {
			std::cout << YELLOW << "Client FD " << it->first << " timed out after " << TIMEOUT_SECONDS << " seconds" << RESET << std::endl;
			int clientFd = it->first;
			it = clientTimeouts_.erase(it); // Erase first
			
		// change: send timeout response before closing connection
		sendTimeoutResponse(clientFd);
		closeClientConnection(clientFd);
		} else {
			++it;
		}
	}
}

void Webserv::updateClientTimeout(int clientFd)
{
	clientTimeouts_[clientFd] = time(NULL);
}

void Webserv::sendTimeoutResponse(int clientFd)
{
	// Try to read custom 408 error page
	std::string errorPagePath = "error/408.html";
	std::string errorPageContent;
	
	if (fileExists(errorPagePath)) {
		errorPageContent = readFileContent(errorPagePath);
	} else {
		// Fallback to default response
		errorPageContent = "<html><head><title>408 Request Timeout</title></head>";
		errorPageContent += "<body><h1>408 Request Timeout</h1>";
		errorPageContent += "<p>Your request took too long to complete.</p>";
		errorPageContent += "<p><a href=\"/\">Return to homepage</a></p></body></html>";
	}
	
	// Create HTTP response
	std::string response = "HTTP/1.1 408 Request Timeout\r\n";
	response += "Content-Type: text/html\r\n";
	response += "Content-Length: " + std::to_string(errorPageContent.length()) + "\r\n";
	response += "Connection: close\r\n";
	response += "\r\n";
	response += errorPageContent;
	
	// Send the response
	ssize_t bytesSent = send(clientFd, response.c_str(), response.length(), 0);
	if (bytesSent == -1) {
		// Ignore send errors for timeout responses since we're closing the connection
	}
}

// change: Added CGI timeout management to prevent hanging processes
void Webserv::checkCgiTimeouts()
{
	time_t currentTime = time(NULL);
	const int CGI_TIMEOUT_SECONDS = 30; // 30 seconds for CGI processes
	
	for (auto it = clientCgiMap_.begin(); it != clientCgiMap_.end();) {
		if (it->second && currentTime - it->second->getStartTime() > CGI_TIMEOUT_SECONDS) {
			std::cout << YELLOW << "CGI process for client FD " << it->first << " timed out after " << CGI_TIMEOUT_SECONDS << " seconds" << RESET << std::endl;
			int clientFd = it->first;
			it = clientCgiMap_.erase(it);
			
			// Send timeout response
			sendTimeoutResponse(clientFd);
			closeClientConnection(clientFd);
		} else {
			++it;
		}
	}
}

// change: periodic cleanup of completed CGI processes
void Webserv::checkCompletedCgis()
{
	for (auto it = clientCgiMap_.begin(); it != clientCgiMap_.end();) {
		if (it->second && it->second->isCgiComplete()) {
			// CGI is complete, clean up
			it = clientCgiMap_.erase(it);
		} else {
			++it;
		}
	}
}

void Webserv::handleCgiEvent(int pipeFd, uint32_t events)
{
	// Find the client FD associated with this CGI pipe
	int clientFd = -1;
	for (auto it = clientCgiMap_.begin(); it != clientCgiMap_.end(); ++it) {
		if (it->second && (it->second->getInputPipe() == pipeFd || it->second->getOutputPipe() == pipeFd)) {
			clientFd = it->first;
			break;
		}
	}
	
	if (clientFd == -1) {
		// CGI pipe not found in our map, remove from epoll
		removeFdFromEpoll(pipeFd);
		return;
	}
	
	std::shared_ptr<Cgi> cgi = clientCgiMap_[clientFd];
	if (!cgi) {
		removeFdFromEpoll(pipeFd);
		return;
	}
	
	// change: input pipe (EPOLLOUT event) -> only remove from epoll when input is complete
	if (events & EPOLLOUT && pipeFd == cgi->getInputPipe()) {
		if (cgi->writeToCgiInput()) {
			// Input writing complete, remove from epoll
			removeFdFromEpoll(pipeFd);
		}
	}
	
	// change: output pipe (EPOLLIN event) -> keep in epoll until EPOLLHUP
	if (events & EPOLLIN && pipeFd == cgi->getOutputPipe()) {
		cgi->readFromCgiOutput();
		// change: Check if CGI finished during reading (based on external repo analysis)
		if (cgi->isCgiComplete() && cgi->isOutputComplete() && cgi->isInputComplete()) {
			createCgiResponse(clientFd, cgi);
			return;
		}
	}
	
	// change: EPOLLHUP on output pipe -> only when CGI process finished
	if (events & EPOLLHUP && pipeFd == cgi->getOutputPipe()) {
		std::cout << "DEBUG: EPOLLHUP received on CGI output pipe for client FD " << clientFd << std::endl;
		// Read any remaining output
		cgi->readFromCgiOutput();
		// Mark output as complete
		cgi->markOutputComplete();
		// Force completion if process hasn't been detected as finished yet
		if (!cgi->isCgiComplete()) {
			cgi->forceComplete();
		}
		// Remove from epoll
		removeFdFromEpoll(pipeFd);
		
		// change: Immediately create response when CGI finishes (based on external repo analysis)
		std::cout << "DEBUG: Creating CGI response for client FD " << clientFd << std::endl;
		createCgiResponse(clientFd, cgi);
		return;
	}
	
	// change: Check if CGI is complete and create response (for cases where process finishes without EPOLLHUP)
	if (cgi->isCgiComplete() && cgi->isOutputComplete() && cgi->isInputComplete()) {
		createCgiResponse(clientFd, cgi);
		return;
	}
	
	// Handle other errors
	if (events & (EPOLLERR | EPOLLRDHUP)) {
		removeFdFromEpoll(pipeFd);
	}
}

// change: Extract CGI response creation into separate function for better organization
void Webserv::createCgiResponse(int clientFd, std::shared_ptr<Cgi> cgi)
{
	std::cout << "DEBUG: createCgiResponse called for client FD " << clientFd << std::endl;
	// CGI is completely done, create response
	std::string responseContent = cgi->getCgiOutput();
	int finalStatusCode = cgi->getCgiStatusCode();
	
	std::cout << "DEBUG: CGI output length: " << responseContent.length() << ", status: " << finalStatusCode << std::endl;
	
	if (responseContent.empty()) {
		finalStatusCode = 500;
		responseContent = "<h1>500 Internal Server Error</h1><p>CGI produced empty output</p>";
	}
	
	// Create HTTP response
	Response response;
	response.setStatusCode(finalStatusCode);
	
	// Parse CGI output to extract headers and body
	size_t headerEnd = responseContent.find("\n\n");
	if (headerEnd != std::string::npos) {
		// Extract headers
		std::string headers = responseContent.substr(0, headerEnd);
		std::string body = responseContent.substr(headerEnd + 2);
		
		// Parse headers line by line
		std::istringstream headerStream(headers);
		std::string line;
		while (std::getline(headerStream, line)) {
			size_t colonPos = line.find(":");
			if (colonPos != std::string::npos) {
				std::string headerName = line.substr(0, colonPos);
				std::string headerValue = line.substr(colonPos + 1);
				// Trim whitespace
				headerValue.erase(0, headerValue.find_first_not_of(" \t"));
				headerValue.erase(headerValue.find_last_not_of(" \t\r\n") + 1);
				response.setHeader(headerName, headerValue);
			}
		}
		response.setBody(body);
	} else {
		// No headers found, use entire content as body
		response.setBody(responseContent);
	}
	
	clientResponses_[clientFd] = response.toString();
	
	// Switch to EPOLLOUT to send the response
	epoll_event event_mod;
	event_mod.events = EPOLLOUT | EPOLLRDHUP | EPOLLET;
	event_mod.data.fd = clientFd;
	if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &event_mod) == -1) {
		closeClientConnection(clientFd);
	}
	
	// Clean up CGI
	clientCgiMap_.erase(clientFd);
}


