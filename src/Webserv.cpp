#include "../incl/Webserv.hpp"

// --- Constructor ---
Webserv::Webserv(const Server42& config)
	: allServersConfig_(config), // Initialize const reference
	  epollFd_(-1),              // Initialize epollFd
	  router_(config)            // Initialize Router, passing config
{
	epollFd_ = epoll_create1(0); // Create the single epoll instance
	if (epollFd_ == -1) {
		throw std::runtime_error("Webserv: Failed to create epoll instance");
	}
	events_.resize(MAX_EVENTS); // Resize event buffer
	std::cout << "Webserv orchestrator created with epoll FD: " << epollFd_ << std::endl;
}

// --- Destructor ---
Webserv::~Webserv() {
	// Close the central epoll instance FD
	if (epollFd_ != -1) {
		close(epollFd_);
	}

	// Close all remaining client FDs if any are still open (robust cleanup)
	for (std::map<int, Request>::iterator it = clientRequests_.begin(); it != clientRequests_.end(); ++it) {
		close(it->first); // Close the client socket
	}
	clientRequests_.clear();    // Clear all client state maps
	clientResponses_.clear();
	listenerMap_.clear();       // Clear listener map (listeners closed by SingleServer destructor)
	clientToServerMap_.clear();

	std::cout << "Webserv orchestrator destructed." << std::endl;
}

// --- Main Server Control Methods ---

// Sets up all listening sockets and adds them to the central epoll instance
void Webserv::start() {
	std::cout << "Starting Webserv..." << std::endl;
	const std::vector<SingleServer>& servers = allServersConfig_.getServers(); // Get server configs

	if (servers.empty()) {
		throw std::runtime_error("No server configurations loaded. Please check your config file.");
	}

	// Initialize each SingleServer's listening socket
	for (size_t i = 0; i < servers.size(); ++i) {
		SingleServer& server = const_cast<SingleServer&>(servers[i]); // Need non-const ref to call initSocket()
		try {
			server.initSocket(); // This sets up server.serverFd_ and makes it non-blocking
			addFdToEpoll(server.getServFd(), EPOLLIN); // Add listener FD to central epoll
			listenerMap_[server.getServFd()] = &server; // Map listener FD to its config
			std::cout << GREEN << "  -> Server '" << server.getServName() << "' listening on port "
					  << server.getServPortInt() << " (FD: " << server.getServFd() << ")" << RESET << std::endl;
		} catch (const std::exception& e) {
			std::cerr << RED << "Error initializing server on port " << server.getServPortInt()
					  << ": " << e.what() << RESET << std::endl;
			// Decide whether to continue or exit if a server fails to initialize
			// For now, we continue, but consider program termination if a critical listener fails.
		}
	}
	std::cout << "All listeners set up. Running main event loop." << std::endl;
	runEventLoop(); // Start the central event loop
}

// Runs the central epoll_wait loop, dispatching events
void Webserv::runEventLoop() {
	while (true) {
		int numEvents = epoll_wait(epollFd_, events_.data(), events_.size(), -1); // -1 for infinite timeout
		if (numEvents == -1) {
			std::cerr << RED << "epoll_wait() failed: " << strerror(errno) << RESET << std::endl;
			throw std::runtime_error("epoll_wait failed, critical error.");
		}

		for (int i = 0; i < numEvents; ++i) {
			int currentFd = events_[i].data.fd;
			uint32_t currentEvents = events_[i].events;

			// Handle errors or hangup on the FD
			if (currentEvents & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
				std::cerr << RED << "Epoll error, hangup or remote shutdown on FD " << currentFd << RESET << std::endl;
				closeClientConnection(currentFd); // Clean up the problematic connection
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

// --- Private Helper Implementations (Epoll Management) ---

void Webserv::addFdToEpoll(int fd, uint32_t events) {
	epoll_event event;
	event.events = events | EPOLLRDHUP | EPOLLET; // Always include RDHUP for graceful disconnect, ET for edge-triggered
	event.data.fd = fd;
	if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) == -1) {
		std::cerr << RED << "epoll_ctl(ADD, FD " << fd << ") failed: " << strerror(errno) << RESET << std::endl;
		throw std::runtime_error("Failed to add FD to epoll");
	}
	// std::cout << "Added FD " << fd << " to epoll for events " << events << std::endl; // Debug
}

void Webserv::removeFdFromEpoll(int fd) {
	if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, NULL) == -1) {
		std::cerr << RED << "epoll_ctl(DEL, FD " << fd << ") failed: " << strerror(errno) << RESET << std::endl;
	}
	// std::cout << "Removed FD " << fd << " from epoll" << std::endl; // Debug
}

void Webserv::closeClientConnection(int clientFd) {
	removeFdFromEpoll(clientFd); // Remove from epoll monitoring
	close(clientFd);             // Close the actual socket FD

	clientRequests_.erase(clientFd);    // Remove client's request state
	clientResponses_.erase(clientFd);   // Remove client's response state
	clientToServerMap_.erase(clientFd); // Remove mapping to server config

	std::cout << YELLOW << "Closed connection for FD: " << clientFd << RESET << std::endl;
}

// --- Private Helper Implementations (Event Handling) ---

// Handles a new incoming connection on a listener socket
void Webserv::handleNewConnection(int listenerFd) {
	struct sockaddr_storage client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	int clientFd = accept(listenerFd, (struct sockaddr *)&client_addr, &client_addr_len);
	if (clientFd == -1) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) { // Expected for non-blocking accept
			std::cerr << RED << "Accept failed on listener FD " << listenerFd << ": " << strerror(errno) << RESET << std::endl;
		}
		return;
	}

	// Map this client to the SingleServer config that accepted it (for routing)
	clientToServerMap_[clientFd] = listenerMap_[listenerFd];
	clientRequests_.insert(std::make_pair(clientFd, Request())); // Initialize a Request object for this client

	addFdToEpoll(clientFd, EPOLLIN); // Add client FD to epoll for reading
	std::cout << GREEN << "Accepted new client (FD: " << clientFd << ") on listener " << listenerFd << RESET << std::endl;
}

// Handles incoming data from a client socket
void Webserv::handleClientRead(int clientFd) {
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, sizeof(buffer));

	// Get the Request object for this client
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
	} else if (bytesReceived == 0) { // Client disconnected gracefully
		std::cout << "Client (FD: " << clientFd << ") disconnected gracefully." << std::endl;
		closeClientConnection(clientFd);
		return;
	} else { // Data received
		request_it->second.appendRawData(buffer, bytesReceived);

		// Loop to process all complete requests that might be in the buffer
		while (request_it->second.processRequestData()) {
			// Request is fully parsed into request_it->second

			// Get the SingleServer config associated with this client for routing
			const SingleServer* clientServer = clientToServerMap_[clientFd];
			if (!clientServer) {
				std::cerr << RED << "Error: No server config found for client FD " << clientFd << ". Closing." << RESET << std::endl;
				closeClientConnection(clientFd);
				return;
			}

			// --- Perform URL Routing ---
			ActionParameters action = router_.routeRequest(request_it->second, clientServer->getServPortInt());

			// --- Perform Action & Generate Response ---
			std::string responseContent = ""; // Content for the response body
			int finalStatusCode = 0;        // Status determined by action execution

			// Handle immediate errors from Router
			if (action.errorCode != 0) {
				finalStatusCode = action.errorCode;
				responseContent = clientServer->getErrorPagePath(action.errorCode); // Get custom error page path
				if (responseContent.empty()) { // If no custom page, provide a generic one
					responseContent = "<h1>Error " + std::to_string(finalStatusCode) + "</h1><p>The requested resource could not be processed.</p>";
				} else { // Read custom error page file content
					responseContent = readFileContent(responseContent); // Webserv reads the error page
				}
			}
			// --- Specific Action Execution (CGI, POST/Upload, DELETE) ---
			else if (action.isCGI || action.isUpload || action.isDeleteOperation) {
				// do something??
			}
			// --- "Build a Usual Response" (Static File, Redirect, Unhandled GET/HEAD) ---
			else if (action.isRedirect) {
				finalStatusCode = action.redirectCode;
				// responseContent holds the redirect URL, Response::buildFromAction handles Location header
				responseContent = action.redirectUrl; 
			}
			else if (action.isStaticFile) { // This includes GET/HEAD for files and autoindex directories
				if (action.isAutoindex) {
					responseContent = generateDirectoryListing(action.filePath);
					finalStatusCode = 200;
				} else {
					responseContent = readFileContent(action.filePath);
					if (responseContent.empty() && fileExists(action.filePath)) { // Check if empty means read error for non-empty file
						finalStatusCode = 500; // Read error
						responseContent = "<h1>500 Internal Server Error</h1><p>Failed to read static file: " + action.filePath + "</p>";
					} else {
						finalStatusCode = 200;
					}
				}
			}
			else { // Fallback for unhandled paths from Router (e.g., 501 Not Implemented)
				finalStatusCode = action.errorCode; // Use errorCode from router, if any
				if (finalStatusCode == 0) finalStatusCode = 500; // Default to 500 if no specific error
				responseContent = "<h1>" + std::to_string(finalStatusCode) + " Internal Server Error</h1><p>Unhandled action type after routing.</p>";
			}

			// --- Call Response Builder ---
			Response response_object;
			// Pass the action, the content produced by action execution, and the final status code
			response_object.buildFromAction(action, responseContent, finalStatusCode); 

			// --- Get the final raw response string ---
			std::string rawResponseString = response_object.toString();

			// --- Store and Prepare for Sending ---
			clientResponses_[clientFd] = rawResponseString; // Store the complete raw response

			// Change epoll event to EPOLLOUT to start sending the response
			epoll_event event_mod;
			event_mod.events = EPOLLOUT | EPOLLIN | EPOLLRDHUP | EPOLLET; // Want to write AND continue reading
			event_mod.data.fd = clientFd;
			if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &event_mod) == -1) {
				std::cerr << RED << "epoll_ctl(MOD, EPOLLOUT|EPOLLIN) failed for FD " << clientFd << ": " << strerror(errno) << RESET << std::endl;
				closeClientConnection(clientFd);
			}
			request_it->second.clearParsedRequest(); // Clear request buffer for next request on this connection
		}
		// If while loop finishes, either all complete requests processed, or buffer holds incomplete request.
	}
}

// Handles sending data to a client socket
void Webserv::handleClientWrite(int clientFd) {
	std::map<int, std::string>::iterator response_it = clientResponses_.find(clientFd);
	if (response_it == clientResponses_.end() || response_it->second.empty()) {
		// No response pending or already sent. Switch back to EPOLLIN if connection is persistent.
		epoll_event event_mod;
		event_mod.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
		event_mod.data.fd = clientFd;
		if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &event_mod) == -1) {
			std::cerr << RED << "epoll_ctl(MOD, EPOLLIN) failed for FD " << clientFd << ": " << strerror(errno) << RESET << std::endl;
			closeClientConnection(clientFd);
		}
		return;
	}

	// Attempt to send data
	ssize_t bytesSent = send(clientFd, response_it->second.c_str(), response_it->second.length(), 0);

	if (bytesSent == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			// Send buffer full, try again later. Do nothing, EPOLLOUT will trigger again.
			// std::cerr << "Send buffer full for FD " << clientFd << ", waiting for EPOLLOUT." << std::endl; // Debug
		} else { // Real error during send
			std::cerr << RED << "Send failed on FD " << clientFd << ": " << strerror(errno) << RESET << std::endl;
			closeClientConnection(clientFd);
		}
	} else {
		// Data sent. Remove sent bytes from buffer.
		response_it->second.erase(0, bytesSent);

		if (response_it->second.empty()) {
			// All data sent.
			std::cout << GREEN << "All response data sent for FD " << clientFd << "." << RESET << std::endl;
			clientResponses_.erase(clientFd); // Remove response from map

			// Connection is persistent by default (HTTP/1.1), switch back to EPOLLIN
			epoll_event event_mod;
			event_mod.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
			event_mod.data.fd = clientFd;
			if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &event_mod) == -1) {
				std::cerr << RED << "epoll_ctl(MOD, EPOLLIN) failed for FD " << clientFd << ": " << strerror(errno) << RESET << std::endl;
				closeClientConnection(clientFd);
			}
		}
	}
}

// --- Private Helper Implementations (File System Operations) ---

// Reads content of a file into a string
std::string Webserv::readFileContent(const std::string& path) const {
	std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		std::cerr << RED << "Error: Could not open file for reading: " << path << RESET << std::endl;
		return ""; // Return empty string on failure
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	file.close();
	return buffer.str();
}

// Determines the MIME type based on file extension
// std::string Webserv::getMimeType(const std::string& filePath) const {
// 	size_t dotPos = filePath.rfind('.');
// 	if (dotPos == std::string::npos) return "application/octet-stream";

// 	std::string ext = filePath.substr(dotPos);
// 	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower); // Case-insensitive

// 	if (ext == ".html" || ext == ".htm") return "text/html";
// 	else if (ext == ".css") return "text/css";
// 	else if (ext == ".js") return "application/javascript";
// 	else if (ext == ".json") return "application/json";
// 	else if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
// 	else if (ext == ".png") return "image/png";
// 	else if (ext == ".gif") return "image/gif";
// 	else if (ext == ".ico") return "image/x-icon";
// 	else if (ext == ".txt") return "text/plain";
// 	else if (ext == ".pdf") return "application/pdf";
// 	else if (ext == ".xml") return "application/xml";
// 	else if (ext == ".mp3") return "audio/mpeg";
// 	else if (ext == ".mp4") return "video/mp4";
// 	// Add more as needed
	
// 	return "application/octet-stream";
// }

// Generates an HTML listing for a directory
// std::string Webserv::generateDirectoryListing(const std::string& directoryPath) const {
// 	std::stringstream html_listing;
// 	html_listing << "<!DOCTYPE html>\r\n"
// 				 << "<html>\r\n"
// 				 << "<head><title>Directory Listing for " << directoryPath << "</title></head>\r\n"
// 				 << "<body>\r\n"
// 				 << "<h1>Directory Listing for " << directoryPath << "</h1>\r\n"
// 				 << "<ul>\r\n";

// 	DIR *dir = opendir(directoryPath.c_str());
// 	if (dir == NULL) {
// 		std::cerr << RED << "Error opening directory for listing: " << directoryPath << ": " << strerror(errno) << RESET << std::endl;
// 		html_listing << "<p>Error: Could not open directory.</p>\r\n";
// 	} else {
// 		struct dirent *entry;
// 		while ((entry = readdir(dir)) != NULL) {
// 			std::string name = entry->d_name;
// 			if (name == "." || name == "..") continue;

// 			std::string fullEntryPath = directoryPath;
// 			if (fullEntryPath.back() != '/') fullEntryPath += "/";
// 			fullEntryPath += name;

// 			html_listing << "<li><a href=\"" << name;
// 			struct stat entry_stat;
// 			if (stat(fullEntryPath.c_str(), &entry_stat) == 0 && S_ISDIR(entry_stat.st_mode)) {
// 				html_listing << "/";
// 			}
// 			html_listing << "\">" << name;
// 			if (stat(fullEntryPath.c_str(), &entry_stat) == 0 && S_ISDIR(entry_stat.st_mode)) {
// 				html_listing << "/";
// 			}
// 			html_listing << "</a></li>\r\n";
// 		}
// 		closedir(dir);
// 	}

// 	html_listing << "</ul>\r\n"
// 				 << "</body>\r\n"
// 				 << "</html>\r\n";
// 	return html_listing.str();
// }

// Checks if a path points to a regular file
bool Webserv::fileExists(const std::string& path) const {
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
}

// Checks if a path points to a directory
bool Webserv::isDirectory(const std::string& path) const {
	struct stat buffer;
	return (stat(path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode));
}

// Checks if a path has write access
bool Webserv::hasWriteAccess(const std::string& path) const {
	return access(path.c_str(), W_OK) == 0;
}

// Checks if a path has execute access (for CGI scripts)
// bool Webserv::isExecutable(const std::string& path) const {
// 	struct stat buffer;
// 	return (stat(path.c_str(), &buffer) == 0 && (buffer.st_mode & S_IXUSR || buffer.st_mode & S_IXGRP || buffer.st_mode & S_IXOTH));
// }
