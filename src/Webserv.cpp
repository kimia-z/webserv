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
		// Check for client timeouts before waiting for events
		checkClientTimeouts();
		
		std::cout << "DEBUG: Starting event loop iteration, active clients: " << clientRequests_.size() 
				  << ", CGI processes: " << clientCgiMap_.size() << std::endl;
		int numEvents = epoll_wait(epollFd_, events_.data(), events_.size(), 5000); // 5 second timeout
		std::cout << "DEBUG: epoll_wait returned " << numEvents << " events" << std::endl;
		if (numEvents == -1) {
			if (errno == EINTR) { // Check if the error was due to an interrupted system call (SIGINT)
				std::cerr << "epoll_wait was interrupted by a signal." << std::endl;
				continue;
			} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
				// Timeout occurred, continue to check timeouts
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
	std::cout << "DEBUG: removeFdFromEpoll called for FD " << fd << ", epollFd_ = " << epollFd_ << std::endl;
	if (epollFd_ < 0) {
		std::cerr << "DEBUG: Invalid epollFd_ = " << epollFd_ << std::endl;
		return;
	}
	if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, NULL) == -1) {
		std::cerr << RED << "epoll_ctl(DEL, FD " << fd << ") failed: " << strerror(errno) << RESET << std::endl;
	} else {
		std::cout << "DEBUG: Successfully removed FD " << fd << " from epoll" << std::endl;
	}
}

void Webserv::closeClientConnection(int clientFd)
{
	std::cout << "DEBUG: Starting cleanup for FD " << clientFd << std::endl;
	
	// Clean up CGI process first if exists
	std::cout << "DEBUG: Checking for CGI process..." << std::endl;
	if (clientCgiMap_.find(clientFd) != clientCgiMap_.end()) {
		std::cout << "DEBUG: Cleaning up CGI process for FD " << clientFd << std::endl;
		try {
			clientCgiMap_.erase(clientFd);
			std::cout << "DEBUG: CGI cleanup completed" << std::endl;
		} catch (const std::exception& e) {
			std::cerr << "DEBUG: Error during CGI cleanup: " << e.what() << std::endl;
		}
	} else {
		std::cout << "DEBUG: No CGI process found for FD " << clientFd << std::endl;
	}

	// Remove from epoll monitoring
	std::cout << "DEBUG: Removing FD from epoll..." << std::endl;
	try {
		removeFdFromEpoll(clientFd);
		std::cout << "DEBUG: FD removed from epoll" << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "DEBUG: Error removing FD from epoll: " << e.what() << std::endl;
	}
	
	// Close the actual socket FD
	std::cout << "DEBUG: Closing socket FD..." << std::endl;
	if (clientFd > 0) {
		close(clientFd);
		std::cout << "DEBUG: Socket closed" << std::endl;
	}

	// Remove client data from maps
	std::cout << "DEBUG: Removing client data from maps..." << std::endl;
	clientRequests_.erase(clientFd);	// Remove client's request state
	clientResponses_.erase(clientFd);	// Remove client's response state
	// Note: clientTimeouts_ is erased in the timeout loop, not here
	clientToServerMap_.erase(clientFd);	// Remove mapping to server config
	std::cout << "DEBUG: Client data removed from maps" << std::endl;

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

	std::cout << "DEBUG: handleClientRead called for FD " << clientFd << std::endl;
	
	// Check if client already has a response pending
	if (clientResponses_.find(clientFd) != clientResponses_.end()) {
		std::cout << "DEBUG: Client FD " << clientFd << " already has response pending, ignoring read" << std::endl;
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
		std::cout << "DEBUG: Client FD " << clientFd << " request already complete, ignoring read" << std::endl;
		return;
	}
	
	std::cout << "DEBUG: Client FD " << clientFd << " proceeding with read - no response pending, request not complete" << std::endl;
	ssize_t bytesReceived = recv(clientFd, buffer, sizeof(buffer), 0);
	std::cout << "DEBUG: Received " << bytesReceived << " bytes from client FD " << clientFd << std::endl;
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
		std::cout << "DEBUG: Appended data to request buffer. Total buffer size: " << request_it->second.getRawBuffer().length() 
				  << " bytes, Body size: " << request_it->second.getBody().length() << " bytes" << std::endl;

		try{
			while (request_it->second.processRequestData())
			{
				std::cout << "DEBUG: Processing request data for FD " << clientFd << std::endl;
				const SingleServer* clientServer = clientToServerMap_[clientFd];
				if (!clientServer) {
					std::cerr << RED << "Error: No server config found for client FD " << clientFd << ". Closing." << RESET << std::endl;
					closeClientConnection(clientFd);
					return;
				}
				
				Router router(allServersConfig_);
				ActionParameters action = router.routeRequest(request_it->second, clientServer->getServPortInt());
				std::cout << "DEBUG: Request method: " << request_it->second.getMethod() << ", Path: " << request_it->second.getPath() << std::endl;
				std::cout << "DEBUG: Router returned - isCGI: " << action.isCGI << ", isUpload: " << action.isUpload << ", cgiScriptPath: " << action.cgiScriptPath << std::endl;

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
					std::cout << "DEBUG: Entering CGI handling for FD " << clientFd << std::endl;
					std::shared_ptr<Cgi>	cgi = clientCgiMap_[clientFd];
					if (!cgi) {
						std::cout << "DEBUG: Creating new CGI object with script: " << action.cgiScriptPath << std::endl;
						clientCgiMap_[clientFd] = std::make_shared<Cgi>(request_it->second, action.cgiScriptPath, 
							clientServer->getServName(), clientServer->getServPortInt());
						cgi = clientCgiMap_[clientFd];
					}
					
					// Set upload directory and max file size for CGI
					if (!action.uploadTargetDir.empty()) {
						std::cout << "DEBUG: Setting upload directory to: " << action.uploadTargetDir << std::endl;
						cgi->setUploadDir(action.uploadTargetDir);
					}
					std::cout << "DEBUG: Setting max file size to: " << clientServer->getMaxBodySize() << std::endl;
					cgi->setMaxFileSize(clientServer->getMaxBodySize());
					
					std::cout << "DEBUG: Starting CGI process..." << std::endl;
					cgi->startCgi([this](int fd, uint32_t events) {
						std::cout << "DEBUG: Adding FD " << fd << " to epoll with events " << events << std::endl;
						addFdToEpoll(fd, events);
					});
					std::cout << GREEN << "Started CGI for client FD " << clientFd << " with script: " << action.cgiScriptPath << RESET << std::endl;
					
					// For now, execute CGI synchronously but without select()
					// TODO: Make this fully non-blocking in the main event loop
					try {
						cgi->setScriptPath(action.cgiScriptPath);
						std::cout << "DEBUG: About to call runCgi()..." << std::endl;
						responseContent = cgi->runCgi();
						finalStatusCode = cgi->getCgiStatusCode();
						std::cout << "DEBUG: CGI completed successfully. Status: " << finalStatusCode << ", Output length: " << responseContent.length() << std::endl;
					}
					catch (const Cgi::CgiException& e) {
						std::cerr << RED << "CGI Exception: " << e.what() << RESET << std::endl;
						finalStatusCode = cgi->getCgiStatusCode(); // Internal Server Error
						responseContent = "<h1>500 Internal Server Error</h1><p>CGI execution failed: " + std::string(e.what()) + "</p>";
					}
					// Mark request as complete after CGI processing
					request_it->second.markComplete();
					
					// Create response from CGI output
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
					
					// Switch to EPOLLOUT to send the response (remove EPOLLIN)
					epoll_event event_mod;
					event_mod.events = EPOLLOUT | EPOLLRDHUP | EPOLLET; // No EPOLLIN!
					event_mod.data.fd = clientFd;
					if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &event_mod) == -1) {
						closeClientConnection(clientFd);
					}
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
			}
		} catch (const HttpException& e) {
			std::cerr << RED << "HTTP Exception: " << e.what() << RESET << std::endl;
			Response errorResponse;
			
			// Use custom error page for 413 errors
			if (e.getCode() == 413) {
				std::string errorPagePath = "error/413.html";
				if (fileExists(errorPagePath)) {
					std::string errorPageContent = readFileContent(errorPagePath);
					std::cout << "DEBUG: 413 error page content length: " << errorPageContent.length() << std::endl;
					errorResponse.buildErrorResponse(413, errorPageContent);
					std::cout << "DEBUG: 413 error response built, status: " << errorResponse.getStatusCode() << std::endl;
				} else {
					std::cout << "DEBUG: 413 error page not found, using default" << std::endl;
					errorResponse.setStatusCode(413);
					errorResponse.setBody("<h1>413 Payload Too Large</h1><p>The request body exceeds the maximum allowed size.</p>");
				}
			} else {
				errorResponse.setStatusCode(e.getCode());
				errorResponse.setBody(e.getMessage());
			}
			
			// Mark request as complete to prevent further data reading
			request_it->second.markComplete();
			std::cout << "DEBUG: Marked request as complete for FD " << clientFd << std::endl;

			// Serialize the error response and add it to the clientResponses_ map
            std::string errorResponseString = errorResponse.toString();
            clientResponses_[clientFd] = errorResponseString;
            std::cout << "DEBUG: 413 error response stored, length: " << clientResponses_[clientFd].length() << std::endl;
            std::cout << "DEBUG: 413 error response content:\n" << clientResponses_[clientFd] << std::endl;

            // Change epoll event to EPOLLOUT to start sending the error response
            epoll_event event_mod;
            event_mod.events = EPOLLOUT | EPOLLRDHUP | EPOLLET;
            event_mod.data.fd = clientFd;
            std::cout << "DEBUG: Switching FD " << clientFd << " to EPOLLOUT mode (removing EPOLLIN)" << std::endl;
            if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &event_mod) == -1) {
                std::cerr << RED << "Failed to switch FD " << clientFd << " to EPOLLOUT: " << strerror(errno) << RESET << std::endl;
                closeClientConnection(clientFd);
			
			} else {
				std::cout << "DEBUG: Successfully switched FD " << clientFd << " to EPOLLOUT mode" << std::endl;
				// Debug: Check if response exists
				std::map<int, std::string>::iterator debug_it = clientResponses_.find(clientFd);
				if (debug_it != clientResponses_.end()) {
					std::cout << "DEBUG: Response found for FD " << clientFd << ", length: " << debug_it->second.length() << std::endl;
					std::cout << "DEBUG: Response preview: " << debug_it->second.substr(0, 100) << "..." << std::endl;
				} else {
					std::cout << "DEBUG: No response found for FD " << clientFd << std::endl;
				}
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
	std::cout << "DEBUG: handleClientWrite called for FD " << clientFd << std::endl;
	std::map<int, std::string>::iterator response_it = clientResponses_.find(clientFd);
	if (response_it == clientResponses_.end()) {
		std::cout << "DEBUG: No response found for FD " << clientFd << " in handleClientWrite" << std::endl;
		closeClientConnection(clientFd);
		return;
	}
	if (response_it->second.empty()) {
		std::cout << "DEBUG: Response is empty for FD " << clientFd << " in handleClientWrite" << std::endl;
		closeClientConnection(clientFd);
		return;
	}
	std::cout << "DEBUG: Response found for FD " << clientFd << ", length: " << response_it->second.length() << std::endl;

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

			// // Switch back to EPOLLIN
			// epoll_event event_mod;
			// event_mod.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
			// event_mod.data.fd = clientFd;
			// if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &event_mod) == -1) {
			// 	closeClientConnection(clientFd);
			// }
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

void Webserv::checkClientTimeouts()
{
	time_t currentTime = time(NULL);
	const int TIMEOUT_SECONDS = 1; // 5 minute timeout for large uploads
	
	for (std::map<int, time_t>::iterator it = clientTimeouts_.begin(); it != clientTimeouts_.end();) {
		if (currentTime - it->second > TIMEOUT_SECONDS) {
			std::cout << YELLOW << "Client FD " << it->first << " timed out after " << TIMEOUT_SECONDS << " seconds" << RESET << std::endl;
			int clientFd = it->first;
			it = clientTimeouts_.erase(it); // Erase first
			
			// Send timeout response before closing
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
	std::cout << "DEBUG: Sending timeout response for FD " << clientFd << std::endl;
	
	// Try to read custom 408 error page
	std::string errorPagePath = "error/408.html";
	std::string errorPageContent;
	
	if (fileExists(errorPagePath)) {
		errorPageContent = readFileContent(errorPagePath);
		std::cout << "DEBUG: Using custom 408 error page, length: " << errorPageContent.length() << std::endl;
	} else {
		// Fallback to default response
		errorPageContent = "<html><head><title>408 Request Timeout</title></head>";
		errorPageContent += "<body><h1>408 Request Timeout</h1>";
		errorPageContent += "<p>Your request took too long to complete.</p>";
		errorPageContent += "<p><a href=\"/\">Return to homepage</a></p></body></html>";
		std::cout << "DEBUG: Using default 408 error page" << std::endl;
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
		std::cerr << "DEBUG: Failed to send timeout response: " << strerror(errno) << std::endl;
	} else {
		std::cout << "DEBUG: Timeout response sent (" << bytesSent << " bytes)" << std::endl;
	}
}

