/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   SingleServer.cpp                                     :+:    :+:            */
/*                                                     +:+                    */
/*   By: kziari <kziari@student.42.fr>                +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/05/13 13:30:17 by mstencel      #+#    #+#                 */
/*   Updated: 2025/05/16 13:10:55 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../incl/SingleServer.hpp"

//basic canonical form

SingleServer::SingleServer():
	serverName_(""),
	locations_(),
	// serverHost_(""),
	serverRoot_(""),
	serverIP_(""),
	serverPortString_("8081"), //temp
	serverPortInt_(-1), // temp
	serverFd_(-1),
	clientFd_(-1),
	maxBodySize_(-1),
	errorPages_(),
	res_(nullptr),
	epollFd_(-1)
	{
	// std::cout << "SingleServer was constructed" << std::endl;
}

SingleServer::SingleServer(int port):
	serverName_(""),
	locations_(),
	// serverHost_(""),
	serverRoot_(""),
	serverIP_(""),
	serverPortString_(std::to_string(port)),
	serverPortInt_(port),
	serverFd_(-1),
	clientFd_(-1),
	maxBodySize_(-1),
	errorPages_(),
	res_(nullptr),
	epollFd_(-1)
	{
	// std::cout << "SingleServer with port: " << port << " was constructed" <<std::endl;
}

SingleServer::SingleServer(const SingleServer& copy):
	serverName_(copy.serverName_),
	locations_(copy.locations_),
	// serverHost_(copy.serverHost_),
	serverRoot_(copy.serverRoot_),
	serverIP_(copy.serverIP_),
	serverPortString_(copy.serverPortString_),
	serverPortInt_(copy.serverPortInt_),
	serverFd_(copy.serverFd_),
	clientFd_(copy.clientFd_),
	maxBodySize_(copy.maxBodySize_),
	errorPages_(copy.errorPages_),
	res_(copy.res_),
	epollFd_(-1)
	{
	// std::cout << "SingleServer's copy was constructed" << std::endl;
}

SingleServer&	SingleServer::operator=(const SingleServer& copy) {
	if (this != &copy) {
		serverName_ = copy.serverName_;
		locations_ = copy.locations_;
		// serverHost_ = copy.serverHost_;
		serverRoot_ = copy.serverRoot_;
		serverIP_ = copy.serverIP_;
		serverPortString_ = copy.serverPortString_;
		serverPortInt_ = copy.serverPortInt_;
		serverFd_ =copy.serverFd_;
		clientFd_ = copy.clientFd_;
		maxBodySize_ = copy.maxBodySize_;
		errorPages_ = copy.errorPages_;
		res_ = copy.res_;
		epollFd_ = copy.epollFd_;
	}
	// std::cout << "SingleServer's copy was created with the copy assignment operator" << std::endl;
	return (*this);
}

SingleServer::~SingleServer() {
	if (res_)
		freeaddrinfo(res_);
	if (serverFd_ >= 0)
		close(serverFd_);
	if (epollFd_ >= 0)
		close(epollFd_);
	// std::cout << "SingleServer was destructed" << std::endl;
}

// getters
std::string	SingleServer::getServName() const {
	return (serverName_);
}

std::vector<Location> SingleServer::getLocations() const {
	return (locations_);
}

// std::string	SingleServer::getServHost() const {
// 	return (serverHost_);
// }

std::string	SingleServer::getServRoot() const {
	return (serverRoot_);
}

std::string SingleServer::getServIP() const {
	return (serverIP_);
}

std::string	SingleServer::getServPortString() const {
	return (serverPortString_);
}

int	SingleServer::getServPortInt() const {
	return (serverPortInt_);
}

int	SingleServer::getServFd() const {
	return (serverFd_);
}

int	SingleServer::getClientFd() const {
	return (clientFd_);
}

int	SingleServer::getMaxBodySize() const {
	return (maxBodySize_);
}

const std::unordered_map<int, std::string>	SingleServer::getErrorPages() const {
	return (errorPages_);
}

addrinfo	*SingleServer::getResults() const {
	return (res_);
}

// setters

void	SingleServer::setServName(const std::string& newServName) {
	serverName_ = newServName;
}

void	SingleServer::setLocations(const Location& newLocation) {
	locations_ .push_back(newLocation);
}

// void	SingleServer::setServHost(const std::string& newServHost) {
// 	serverHost_ = newServHost;
// }

void	SingleServer::setServRoot(const std::string& newServRoot) {
	serverRoot_ = newServRoot;
}

void	SingleServer::setServIP(const std::string& newServIP) {
	serverIP_ = newServIP;
}

void	SingleServer::setServPortString(const std::string& newServPortStr) {
	serverPortString_ = newServPortStr;
}

void	SingleServer::setServPortInt(const int& newServPortInt) {
	serverPortInt_ = newServPortInt;
}

void	SingleServer::setResults(addrinfo* newResult) {
	if (res_)
		freeaddrinfo(res_);
	res_ = newResult;
}

void	SingleServer::setServFd(const int& newServFd) {
	serverFd_ = newServFd;
}

void	SingleServer::setClientFd(const int& newClientFd) {
	clientFd_ = newClientFd;
}

void	SingleServer::setMaxBodySize(const int& newMaxBodySize) {
	maxBodySize_ = newMaxBodySize;
}

void	SingleServer::setErrorPages(const int& errorNb, const std::string& newErrorPage) {
	errorPages_[errorNb] = newErrorPage;
}




void	SingleServer::initSocket() {
	struct addrinfo	hints;
	struct addrinfo	*iterationPointer;
	int				status;
	std::string		portString;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; //works for both IPv4 & IPv6
	hints.ai_socktype = SOCK_STREAM; //for the TCP
	hints.ai_flags = AI_PASSIVE; // fills it in with the localhost address
	
	if ((status = getaddrinfo(NULL, serverPortString_.c_str(), &hints, &res_)) != 0) {
		std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
		// Consider throwing an exception or returning an error code
		// rather than just printing and continuing potentially uninitialized.
		throw std::runtime_error("Failed to get address info"); // Example
	}
	// iteration for debugging!
	// char ipstr[INET6_ADDRSTRLEN];
	// for (iterationPointer = res_; iterationPointer != NULL; iterationPointer = iterationPointer->ai_next) {
	// 	void	*addr;
		
	// 	if (iterationPointer->ai_family == AF_INET) { //for IPv4
	// 		struct sockaddr_in *ipv4 = (struct sockaddr_in *)iterationPointer->ai_addr;
	// 		addr = &(ipv4->sin_addr);
	// 	}
	// 	else {//for IPv6
	// 		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)iterationPointer->ai_addr;
	// 		addr = &(ipv6->sin6_addr);
	// 	}
	// 	inet_ntop(iterationPointer->ai_family, addr, ipstr, sizeof ipstr);
	// 	std::cout << "Resolved address: " << ipstr << std::endl;
	// }
	
	for (iterationPointer = res_; iterationPointer != NULL; iterationPointer = iterationPointer->ai_next) {
		serverFd_ = socket(iterationPointer->ai_family, iterationPointer->ai_socktype, iterationPointer->ai_protocol);
		if (serverFd_ == -1) {
			std::cerr << RED << "Socket() failed: " << strerror(errno) << RESET << std::endl;
			continue;
		}
		int	yes = 1;
		if (setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
			std::cerr << RED << "setsockopt() failed: " << strerror(errno) << RESET << std::endl;
			close (serverFd_);
			continue;
		}
		if (bind(serverFd_, iterationPointer->ai_addr, iterationPointer->ai_addrlen) == 0) {
			break ;
		}
		close(serverFd_);
	}

	if (iterationPointer == NULL) {
		std::cerr << RED << "Could not bind to any address: " << strerror(errno) << RESET << std::endl;
		throw std::runtime_error("Failed to bind socket");
	}
	if (listen(serverFd_, 10) == -1) {
		std::cerr << RED << "listen() failed: " << strerror(errno) << RESET << std::endl;
		close(serverFd_);
		throw std::runtime_error("Failed to listen on socket");
	}
	std::cout << "listening on port " << serverPortString_ << std::endl;
}


void SingleServer::eventLoop()
{
	events_.resize(MAX_EVENTS);

	while (true) {
		int numEvents = epoll_wait(epollFd_, events_.data(), events_.size(), -1);
		if (numEvents == -1) {
			throw std::runtime_error("epoll_wait failed");
		}

		for (int i = 0; i < numEvents; ++i) {
			int currentFd = events_[i].data.fd;
			uint32_t currentEvents = events_[i].events;

			if (currentEvents & (EPOLLERR | EPOLLHUP)) {
				std::cerr << RED << "Epoll error or hangup on FD " << currentFd << RESET << std::endl;
				removeFdFromEpoll(currentFd);
				close(currentFd);
				clients_requests_.erase(currentFd);
				continue;
			}

			if (currentFd == serverFd_) {
				struct sockaddr_storage client_addr;
				socklen_t client_addr_len = sizeof(client_addr);
				int clientFd = accept(serverFd_, (struct sockaddr *)&client_addr, &client_addr_len);
				if (clientFd == -1) {
					if (errno != EAGAIN && errno != EWOULDBLOCK) {
						std::cerr << RED << "Accept failed: " << strerror(errno) << RESET << std::endl;
					}
					continue;
				}
				clients_requests_.insert(std::make_pair(clientFd, Request()));

				addFdToEpoll(clientFd, EPOLLIN | EPOLLRDHUP | EPOLLET);
				std::cout << "Accepted new client (FD: " << clientFd << ")" << std::endl;

			} else {
				if (currentEvents & EPOLLIN) {
					handleClientRead(currentFd);
				}
				// if (currentEvents & EPOLLOUT) {
				//     handleClientWrite(currentFd);
				// }
			}
		}
	}
}

void SingleServer::handleClientRead(int clientFd) {
	char buffer[BUFFER_SIZE];
	memset(buffer, 0, sizeof(buffer));

	std::map<int, Request>::iterator it = clients_requests_.find(clientFd);
	if (it == clients_requests_.end()) {
		std::cerr << RED << "Error: No Request object found for FD " << clientFd << " during read." << RESET << std::endl;
		removeFdFromEpoll(clientFd);
		close(clientFd);
		return;
	}

	ssize_t bytesReceived = recv(clientFd, buffer, sizeof(buffer), 0);

	if (bytesReceived == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return;
		} else {
			std::cerr << RED << "Recv failed on FD " << clientFd << ": " << strerror(errno) << RESET << std::endl;
			removeFdFromEpoll(clientFd);
			close(clientFd);
			clients_requests_.erase(clientFd);
		}
	} else if (bytesReceived == 0) {
		std::cout << "Client (FD: " << clientFd << ") disconnected gracefully." << std::endl;
		removeFdFromEpoll(clientFd);
		close(clientFd);
		clients_requests_.erase(clientFd);
	} else {
		it->second.appendRawData(buffer, bytesReceived);
		while (it->second.processRequestData()) {
			// At this point, it->second contains a fully parsed request.
			it->second.print();
			//check before Response

			std::string dummy_response = "HTTP/1.1 200 OK\r\nContent-Length: 17\r\n\r\nHello from Server";
			ssize_t bytesSent = send(clientFd, dummy_response.c_str(), dummy_response.length(), 0);
			if (bytesSent == -1) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					std::cerr << RED << "Send buffer full on FD " << clientFd << ", will try later (need EPOLLOUT handling)." << RESET << std::endl;
				} else {
					std::cerr << RED << "Send failed on FD " << clientFd << ": " << strerror(errno) << RESET << std::endl;
				}
				removeFdFromEpoll(clientFd);
				close(clientFd);
				clients_requests_.erase(clientFd);
				return;
			} else {
				std::cout << "Sent " << bytesSent << " bytes to FD " << clientFd << std::endl;
			}
			if (bytesSent == static_cast<ssize_t>(dummy_response.length())) {
				 it->second.clearParsedRequest();
				 epoll_event event;
				 event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
				 event.data.fd = clientFd;
				 epoll_ctl(epollFd_, EPOLL_CTL_MOD, clientFd, &event);
			} else {
				std::cerr << "Partial send on FD " << clientFd << ". Remaining " << (dummy_response.length() - bytesSent) << " bytes." << std::endl;
				removeFdFromEpoll(clientFd);
				close(clientFd);
				clients_requests_.erase(clientFd);
				return;
			}
		}
		// If the while loop finishes, either all complete requests were processed,
		// or the remaining data in the buffer is an incomplete request.
		// The function returns, and epoll_wait will continue monitoring.
	}
}