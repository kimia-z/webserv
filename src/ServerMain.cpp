/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerMain.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kziari <kziari@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/13 13:30:17 by mstencel          #+#    #+#             */
/*   Updated: 2025/07/04 18:05:20 by kziari           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incl/ServerMain.hpp"

//basic canonical form

ServerMain::ServerMain():
	serverFd_(-1),
	serverPort_(8081), // I changed the port for now!
	res_(nullptr),
	epollFd_(-1)
	{
	std::cout << "ServerMain was constructed" << std::endl;
}

ServerMain::ServerMain(int port):
	serverFd_(-1),
	res_(nullptr),
	epollFd_(-1)
	{
	serverPort_ = port;
	std::cout << "ServerMain with port: " << port << " was constructed" <<std::endl;
}

ServerMain::ServerMain(const ServerMain& copy):
	serverFd_(copy.serverFd_),
	serverPort_(copy.serverPort_),
	res_(copy.res_),
	epollFd_(copy.epollFd_)
	{
	std::cout << "ServerMain's copy was constructed" << std::endl;
}

ServerMain&	ServerMain::operator=(const ServerMain& copy) {
	if (this != &copy) {
		serverFd_ =copy.serverFd_;
		serverPort_ = copy.serverPort_;
		res_ = copy.res_;
		epollFd_ = copy.epollFd_;
	}
	std::cout << "ServerMain's copy was created with the copy assignment operator" << std::endl;
	return (*this);
}

ServerMain::~ServerMain() {
	if (res_)
		freeaddrinfo(res_);
	if (serverFd_ >= 0)
		close(serverFd_);
	if (epollFd_ >= 0)
		close(epollFd_);
	std::cout << "ServerMain was destructed" << std::endl;
}

// getters
int	ServerMain::getServFd() const {
	return (serverFd_);
}

int	ServerMain::getServPort() const {
	return (serverPort_);
}

addrinfo	*ServerMain::getResults() const {
	return (res_);
}

// setters
void	ServerMain::setServPort(int newServPort) {
	serverPort_ = newServPort;
}

void	ServerMain::setServFd(int newServFd) {
	serverFd_ = newServFd;
}

void	ServerMain::setResults(addrinfo *newResult) {
	if (res_)
		freeaddrinfo(res_);
	res_ = newResult;
}

// Suggestion name for this member function: initializeSocket()/setupSocket()
void	ServerMain::startSocket() {
	struct addrinfo	hints;
	struct addrinfo	*iterationPointer;
	int				status;
	std::string		portString;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; //works for both IPv4 & IPv6
	hints.ai_socktype = SOCK_STREAM; //for the TCP
	hints.ai_flags = AI_PASSIVE; // fills it in with the localhost address
	portString = std::to_string(serverPort_);
	
	if ((status = getaddrinfo(NULL, portString.c_str(), &hints, &res_)) != 0) {
		std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
		// Consider throwing an exception or returning an error code
		// rather than just printing and continuing potentially uninitialized.
		throw std::runtime_error("Failed to get address info"); // Example
	}
	// iteration for debugging!
	for (iterationPointer = res_; iterationPointer != NULL; iterationPointer = iterationPointer->ai_next) {
		void	*addr;
		
		if (iterationPointer->ai_family == AF_INET) { //for IPv4
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)iterationPointer->ai_addr;
			addr = &(ipv4->sin_addr);
		}
		else {//for IPv6
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)iterationPointer->ai_addr;
			addr = &(ipv6->sin6_addr);
		}
	}
	
	for (iterationPointer = res_; iterationPointer != NULL; iterationPointer = iterationPointer->ai_next) {
		serverFd_ = socket(iterationPointer->ai_family, iterationPointer->ai_socktype, iterationPointer->ai_protocol);
		if (serverFd_ == -1) {
			std::cerr << RED << "Socket() failed: " << strerror(errno) << RESET << std::endl;
			continue;
		}
		int	yes = 1;
		if (setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) { // SOL_SOCK for APIs & SO_REUSEADDR for
			std::cerr << RED << "setsockopt() failed: " << strerror(errno) << RESET << std::endl;
			close (serverFd_);
			continue;
		}
		if (bind(serverFd_, iterationPointer->ai_addr, iterationPointer->ai_addrlen) == 0) {
			break ; //bind worked!
		}
		close(serverFd_); // Close socket if bind failed, try next address
	}

	if (iterationPointer == NULL) {
		std::cerr << RED << "Could not bind to any address: " << strerror(errno) << RESET << std::endl;
		throw std::runtime_error("Failed to bind socket"); // Stop program if no address worked
	}
	
	int flags = fcntl(serverFd_, F_GETFL, 0);
	if (flags == -1)
	{
		std::cerr << RED << "fcntl(F_GETFL) failed for server socket: " << strerror(errno) << RESET << std::endl;
		close(serverFd_);
		throw std::runtime_error("Failed to get socket flags");
	}
	flags |= O_NONBLOCK;
	if (fcntl(serverFd_, F_SETFL, flags) == -1)
	{
		std::cerr << RED << "fcntl(F_SETFL, O_NONBLOCK) failed for server socket: " << strerror(errno) << RESET << std::endl;
		close(serverFd_);
		throw std::runtime_error("Failed to set socket flags");
	}
	if (listen(serverFd_, 10) == -1) {
		std::cerr << RED << "listen() failed: " << strerror(errno) << RESET << std::endl;
		close(serverFd_); // Close if listen fails
		throw std::runtime_error("Failed to listen on socket");
	}
	std::cout << "listening on port " << serverPort_ << std::endl;
}

// Suggestion name for this member function: acceptConnections()
// void ServerMain::startConnection() {
// 	struct sockaddr_storage other_addr;
// 	char buffer[30000];
	
// 	socklen_t addr_size = sizeof(other_addr);
	
// 	while(true)
// 	{
// 		clientFd_ = accept(serverFd_, (struct sockaddr *)&other_addr, &addr_size); // Should we call getServFd() as argument?
// 		if (clientFd_ == -1){
// 			std::cerr << RED << "Accept failed: " << strerror(errno) << RESET << std::endl;
// 			//TODO return? how to stop the program - no exit() allowed
// 		}
// 		memset(buffer, 0, sizeof(buffer));
// 		ssize_t bytesReceived = recv(clientFd_, buffer, sizeof(buffer), 0);
		
// 		if (bytesReceived == -1){
// 			std::cerr << RED << "Recv failed: " << strerror(errno) << RESET << std::endl;
// 			close(clientFd_);
// 			close(serverFd_);
// 		}
// 		else{
// 			std::string rawRequest_(buffer, bytesReceived);
// 			// std::cout << GREEN << "Received:" << bytesReceived << RESET << std::endl;
// 			// std::cout << YELLOW << "Client Request:\n" << rawRequest_ << RESET << std::endl;
// 			// ----Parsing the HTTP request----
// 			Request httpRequest(rawRequest_);
// 			httpRequest.print();
// 		}

		
// 		// ----Generate HTTP response-----
// 		// simple HTTP response
// 		std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 17\r\n\r\nHello from Server";
// 		ssize_t	bytesSent = send(clientFd_, response.c_str(), response.length(), 0);
// 		if (bytesSent == -1){
// 			std::cerr << RED << "Send failed: " << strerror(errno) << RESET << std::endl;
// 			close(serverFd_);
// 			close(clientFd_);
// 		}
// 		// Done with client socket
// 		close(clientFd_);
// 	}
// 	// close server socket and free program in destructor!
// }

void ServerMain::eventLoop() {
	
	events_.resize(MAX_EVENTS);
	
	while (true) {
		int numEvents = epoll_wait(epollFd_, events_.data(), events_.size(), -1); // -1 for infinite timeout
		if (numEvents == -1) {
			std::cerr << RED << "epoll_wait() failed: " << strerror(errno) << RESET << std::endl;
			// Handle error, maybe retry or exit if fatal
			// For now, let's throw
			throw std::runtime_error("epoll_wait failed");
		}

		for (int i = 0; i < numEvents; ++i) {
			int currentFd = events_[i].data.fd;
			uint32_t currentEvents = events_[i].events;

			// Handle errors on the FD (EPOLLERR, EPOLLHUP)
			if (currentEvents & (EPOLLERR | EPOLLHUP)) {
				std::cerr << RED << "Epoll error or hangup on FD " << currentFd << RESET << std::endl;
				removeFdFromEpoll(currentFd);
				close(currentFd);
				continue;
			}

			// If it's the listening socket
			if (currentFd == serverFd_) {
				// New connection on listening socket
				struct sockaddr_storage client_addr;
				socklen_t client_addr_len = sizeof(client_addr);
				int clientFd = accept(serverFd_, (struct sockaddr *)&client_addr, &client_addr_len);
				if (clientFd == -1) {
					// EAGAIN/EWOULDBLOCK means no more connections pending right now
					if (errno == EAGAIN || errno == EWOULDBLOCK) {
						// This can happen if multiple events are triggered but not all are true new connections.
						// Or if another thread/process accepted it (not applicable here with single epoll)
					} else {
						std::cerr << RED << "Accept failed: " << strerror(errno) << RESET << std::endl;
						// For a non-blocking accept, if it fails, it's usually not critical.
						// Log and continue the loop.
					}
					continue;
				}
				
				// Make accepted client socket non-blocking ***
				int clientFlags = fcntl(clientFd, F_GETFL, 0);
				if (clientFlags == -1) {
					std::cerr << RED << "fcntl(F_GETFL) failed for clientFd " << clientFd << ": " << strerror(errno) << RESET << std::endl;
					close(clientFd);
					continue;
				}
				clientFlags |= O_NONBLOCK;
				if (fcntl(clientFd, F_SETFL, clientFlags) == -1) {
					std::cerr << RED << "fcntl(F_SETFL, O_NONBLOCK) failed for clientFd " << clientFd << ": " << strerror(errno) << RESET << std::endl;
					close(clientFd);
					continue;
				}

				// Add new client FD to epoll, initially for reading (EPOLLIN)
				addFdToEpoll(clientFd, EPOLLIN);
				std::cout << "Accepted new client (FD: " << clientFd << ")" << std::endl;

			} else {
				// It's a client socket ready for I/O
				// This is where your read/write/request processing logic goes.
				
				if (currentEvents & EPOLLIN) {
					// Data is available to read from clientFd
					char buffer[30000]; // Define buffer size appropriately, maybe dynamically
					memset(buffer, 0, sizeof(buffer));
					ssize_t bytesReceived = recv(currentFd, buffer, sizeof(buffer), 0);
					
					if (bytesReceived == -1) {
						if (errno == EAGAIN || errno == EWOULDBLOCK) {
							// No data available right now, this is normal for non-blocking.
							// The event was likely spurious or data was already read.
						} else {
							std::cerr << RED << "Recv failed on FD " << currentFd << ": " << strerror(errno) << RESET << std::endl;
							removeFdFromEpoll(currentFd);
							close(currentFd);
						}
					} else if (bytesReceived == 0) {
						// Client closed connection
						std::cout << "Client (FD: " << currentFd << ") disconnected gracefully." << std::endl;
						removeFdFromEpoll(currentFd);
						close(currentFd);
					} else {
						// Successfully received data
						std::string rawRequest_(buffer, bytesReceived);
						
						// This is where the Request Parsing and Response Generation logic will go.
						// In a full server:

						// 1. Store `rawRequest_` in a buffer associated with `currentFd`.
						//    IF it->second.requestBuffer EXISTS THEN
						//        APPEND rawRequest_ TO it->second.requestBuffer
						//    ELSE
						//        CREATE new it->second.requestBuffer AND STORE rawRequest_ IN IT
						//    END IF

						// 2. Attempt to parse a complete HTTP request from the buffer (using the Request class).
						//    IF it->second.requestBuffer CONTAINS A COMPLETE HTTP REQUEST THEN
						//        // (This check likely involves looking for \r\n\r\n and Content-Length/chunked data)

						// 3. If a full request is parsed:
						//    a. Process the request (URL routing, method handling, file serving, CGI).
						//       PARSE the complete request from it->second.requestBuffer INTO it->second.httpRequest
						//       DETERMINE the appropriate SERVER_BLOCK based on Host header and Port
						//       FIND the BEST_MATCHING_ROUTE within the SERVER_BLOCK based on URL PATH
						//       CHECK if the HTTP_METHOD (GET, POST, DELETE) is ALLOWED for the BEST_MATCHING_ROUTE
						//       IF METHOD_NOT_ALLOWED THEN
						//           GENERATE 405 Method Not Allowed RESPONSE
						//       ELSE IF ROUTE_IS_REDIRECT THEN
						//           GENERATE 3xx Redirect RESPONSE
						//       ELSE IF REQUEST_BODY_EXCEEDS_MAX_SIZE THEN
						//           GENERATE 413 Payload Too Large RESPONSE
						//       ELSE IF ROUTE_SERVES_STATIC_FILES THEN
						//           RESOLVE FILE_PATH
						//           IF FILE_EXISTS THEN
						//               GENERATE 200 OK RESPONSE with FILE_CONTENT
						//           ELSE IF IS_DIRECTORY AND DIRECTORY_LISTING_ENABLED THEN
						//               GENERATE 200 OK RESPONSE with DIRECTORY_LISTING_HTML
						//           ELSE
						//               GENERATE 404 Not Found OR 403 Forbidden RESPONSE
						//           END IF
						//       ELSE IF ROUTE_IS_CGI THEN
						//           EXECUTE CGI PROCESS (fork/execve/pipe)
						//           GENERATE RESPONSE with CGI_OUTPUT
						//       ELSE IF ROUTE_HANDLES_UPLOADS THEN
						//           PARSE UPLOAD_DATA
						//           SAVE UPLOADED_FILE_TO_DISK
						//           GENERATE 200 OK OR 201 Created RESPONSE
						//       ELSE IF METHOD_IS_DELETE THEN
						//           DELETE FILE_FROM_DISK
						//           GENERATE 204 No Content OR 404 Not Found RESPONSE
						//       ELSE
						//           GENERATE 500 Internal Server Error RESPONSE (for unhandled cases)
						//       END IF
						//       ENSURE DEFAULT_ERROR_PAGES are used for error responses if custom ones aren't set

						//    b. Prepare the HTTP response.
						//       CONVERT the GENERATED_RESPONSE_OBJECT into RAW_HTTP_RESPONSE_STRING

						//    c. Store the response data in a buffer associated with `currentFd`.
						//       STORE RAW_HTTP_RESPONSE_STRING IN it->second.responseBuffer

						//    d. Change the epoll event for `currentFd` from `EPOLLIN` to `EPOLLOUT`**
						//       MODIFY EPOLL_CTL for currentFd to monitor for EPOLLOUT events
						//       (This tells epoll to notify us when we can SEND data)

						//    CLEAR it->second.requestBuffer (to prepare for next request on persistent connection)
						//    // (OPTIONAL: Reset Request object state if needed for next request)

						// END IF (for complete request)

						// (ELSE: If request is NOT yet complete, do nothing, wait for more data on next EPOLLIN)

						
						// For now, let's just send a simple response immediately and then close
						std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 17\r\n\r\nHello from Server";
						ssize_t bytesSent = send(currentFd, response.c_str(), response.length(), 0);
						if (bytesSent == -1) {
							 if (errno == EAGAIN || errno == EWOULDBLOCK) {
								// Send buffer full, need to try again later.
								// You would modify epoll_ctl to add EPOLLOUT and track remaining data to send.
								std::cerr << RED << "Send buffer full on FD " << currentFd << ", will try later." << RESET << std::endl;
								// For now, just close and simplify
								removeFdFromEpoll(currentFd);
								close(currentFd);
							 } else {
								std::cerr << RED << "Send failed on FD " << currentFd << ": " << strerror(errno) << RESET << std::endl;
								removeFdFromEpoll(currentFd);
								close(currentFd);
							 }
						} else {
							std::cout << "Sent " << bytesSent << " bytes to FD " << currentFd << std::endl;
							// Response sent, close the client socket for now
							removeFdFromEpoll(currentFd);
							close(currentFd);
						}
					}
				}
				
				// I will add EPOLLOUT handling here later when I need to send large responses
				// or when the send buffer is full (EAGAIN/EWOULDBLOCK)
				// if (currentEvents & EPOLLOUT) {
				//     // Data can be sent to clientFd
				//     // Call send() for pending data
				//     // If all data sent, remove EPOLLOUT from epoll_ctl for this FD
				// }
			}
		}
	}
}