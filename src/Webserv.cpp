#include "../incl/Webserv.hpp"
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
		int numEvents = epoll_wait(epollFd_, events_.data(), events_.size(), -1);
		if (numEvents == -1) {
            if (errno == EINTR) { // Check if the error was due to an interrupted system call (SIGINT)
                std::cerr << "epoll_wait was interrupted by a signal." << std::endl;
                continue;
            } else {
                std::cerr << RED << "epoll_wait() failed: " << strerror(errno) << RESET << std::endl;
                throw std::runtime_error("epoll_wait failed, critical error.");
		}
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
			//get small parts of request and add also CGI in here
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
					std::shared_ptr<Cgi>	cgi = clientCgiMap_[clientFd];
					std::cout << "in the cgi" << std::endl;
					if (!cgi) {
						clientCgiMap_[clientFd] = std::make_shared<Cgi>(request_it->second, action.cgiScriptPath);
						cgi = clientCgiMap_[clientFd];
					}
					cgi->startCgi([this](int fd, uint32_t events) {
						addFdToEpoll(fd, events);
					});
					std::cout << GREEN << "Started CGI for client FD " << clientFd << " with script: " << action.cgiScriptPath << RESET << std::endl;
					try {
						cgi->setScriptPath(action.cgiScriptPath);
						std::cout << "here?" << std::endl;
						responseContent = cgi->runCgi();
						finalStatusCode = cgi->getCgiStatusCode();
					}
					catch (const Cgi::CgiException& e) {
						std::cerr << RED << "CGI Exception: " << e.what() << RESET << std::endl;
						finalStatusCode = cgi->getCgiStatusCode(); // Internal Server Error
						responseContent = "<h1>500 Internal Server Error</h1><p>CGI execution failed: " + std::string(e.what()) + "</p>";
					}

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

std::string Webserv::generateDirectoryListing(const std::string& directoryPath) const
{
	std::stringstream html_listing;
	html_listing << "<!DOCTYPE html>\r\n"
				 << "<html lang=\"en\">\r\n"
				 << "<head>\r\n"
				 << "    <meta charset=\"UTF-8\">\r\n"
				 << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\r\n"
				 << "    <title>Directory Listing</title>\r\n"
				 << "    <style>\r\n"
				 << "        @import url('https://fonts.googleapis.com/css2?family=Poppins:wght@400;600;700&display=swap');\r\n"
				 << "        \r\n"
				 << "        :root {\r\n"
				 << "            --primary-color: #2c3e50;\r\n"
				 << "            --secondary-color: #3498db;\r\n"
				 << "            --accent-color: #e74c3c;\r\n"
				 << "            --text-color: #f4f4f4;\r\n"
				 << "            --bg-color: #ecf0f1;\r\n"
				 << "            --card-bg: #ffffff;\r\n"
				 << "            --shadow: 0 8px 16px rgba(0, 0, 0, 0.1);\r\n"
				 << "        }\r\n"
				 << "\r\n"
				 << "        body {\r\n"
				 << "            font-family: 'Poppins', sans-serif;\r\n"
				 << "            background-color: var(--bg-color);\r\n"
				 << "            margin: 0;\r\n"
				 << "            padding: 2rem;\r\n"
				 << "            color: var(--primary-color);\r\n"
				 << "        }\r\n"
				 << "\r\n"
				 << "        .container {\r\n"
				 << "            background-color: var(--card-bg);\r\n"
				 << "            padding: 2rem 3rem;\r\n"
				 << "            border-radius: 12px;\r\n"
				 << "            box-shadow: var(--shadow);\r\n"
				 << "            max-width: 800px;\r\n"
				 << "            margin: 2rem auto;\r\n"
				 << "        }\r\n"
				 << "\r\n"
				 << "        h1 {\r\n"
				 << "            font-size: 2.5rem;\r\n"
				 << "            font-weight: 700;\r\n"
				 << "            color: var(--primary-color);\r\n"
				 << "            border-bottom: 2px solid #ddd;\r\n"
				 << "            padding-bottom: 1rem;\r\n"
				 << "            margin-top: 0;\r\n"
				 << "        }\r\n"
				 << "\r\n"
				 << "        ul {\r\n"
				 << "            list-style: none;\r\n"
				 << "            padding: 0;\r\n"
				 << "            margin: 0;\r\n"
				 << "        }\r\n"
				 << "\r\n"
				 << "        li {\r\n"
				 << "            display: flex;\r\n"
				 << "            align-items: center;\r\n"
				 << "            padding: 0.8rem 0;\r\n"
				 << "            border-bottom: 1px solid #eee;\r\n"
				 << "        }\r\n"
				 << "\r\n"
				 << "        li:last-child {\r\n"
				 << "            border-bottom: none;\r\n"
				 << "        }\r\n"
				 << "\r\n"
				 << "        a {\r\n"
				 << "            display: flex;\r\n"
				 << "            align-items: center;\r\n"
				 << "            text-decoration: none;\r\n"
				 << "            color: var(--secondary-color);\r\n"
				 << "            font-weight: 600;\r\n"
				 << "            font-size: 1rem;\r\n"
				 << "            transition: color 0.3s ease;\r\n"
				 << "        }\r\n"
				 << "\r\n"
				 << "        a:hover {\r\n"
				 << "            color: var(--accent-color);\r\n"
				 << "        }\r\n"
				 << "\r\n"
				 << "        .icon {\r\n"
				 << "            font-size: 1.2rem;\r\n"
				 << "            margin-right: 0.8rem;\r\n"
				 << "            color: #7f8c8d;\r\n"
				 << "        }\r\n"
				 << "\r\n"
				 << "        .icon-dir::before {\r\n"
				 << "            content: \"📁\";\r\n"
				 << "        }\r\n"
				 << "\r\n"
				 << "        .icon-file::before {\r\n"
				 << "            content: \"📄\";\r\n"
				 << "        }\r\n"
				 << "\r\n"
				 << "        .footer {\r\n"
				 << "            margin-top: 2rem;\r\n"
				 << "            text-align: center;\r\n"
				 << "            font-size: 0.8rem;\r\n"
				 << "            color: #7f8c8d;\r\n"
				 << "        }\r\n"
				 << "    </style>\r\n"
				 << "</head>\r\n"
				 << "<body>\r\n"
				 << "    <div class=\"container\">\r\n"
				 << "        <h1>Directory Listing for " << directoryPath << "</h1>\r\n"
				 << "        <ul>\r\n"
				 << "            <li>\r\n"
				 << "                <a href=\"../\">\r\n"
				 << "                    <span class=\"icon icon-dir\"></span>\r\n"
				 << "                    .. (Parent Directory)\r\n"
				 << "                </a>\r\n"
				 << "            </li>\r\n";

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
				html_listing << "            <li>\r\n"
							 << "                <a href=\"" << name << "/\">\r\n"
							 << "                    <span class=\"icon icon-dir\"></span>\r\n"
							 << "                    " << name << "/\r\n"
							 << "                </a>\r\n"
							 << "            </li>\r\n";
			} else {
				html_listing << "            <li>\r\n"
							 << "                <a href=\"" << name << "\">\r\n"
							 << "                    <span class=\"icon icon-file\"></span>\r\n"
							 << "                    " << name << "\r\n"
							 << "                </a>\r\n"
							 << "            </li>\r\n";
			}
		}
		closedir(dir);
	}

	html_listing << "        </ul>\r\n"
				 << "    </div>\r\n"
				 << "    <div class=\"footer\">\r\n"
				 << "        Generated by `webserv`.\r\n"
				 << "    </div>\r\n"
				 << "</body>\r\n"
				 << "</html>\r\n";
	return html_listing.str();
}
