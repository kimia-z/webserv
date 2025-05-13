/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ServerMain.cpp                                     :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/05/13 13:30:17 by mstencel      #+#    #+#                 */
/*   Updated: 2025/05/13 15:10:02 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "incl/ServerMain.hpp"

//basic canonical form

ServerMain::ServerMain():
	serverFd_(-1),
	serverPort_(0),
	res_(nullptr)
	{
	std::cout << "ServerMain was constructed" << std::endl;
}

ServerMain::ServerMain(int port):
	serverFd_(-1),
	res_(nullptr)
	{
	serverPort_ = port;
	std::cout << "ServerMain with port: " << port << " was constructed" <<std::endl;
}

ServerMain::ServerMain(const ServerMain& copy):
	serverFd_(copy.serverFd_),
	serverPort_(copy.serverPort_),
	res_(copy.res_)
	{
	std::cout << "ServerMain's copy was constructed" << std::endl;
}

ServerMain&	ServerMain::operator=(const ServerMain& copy) {
	if (this != &copy) {
		serverFd_ =copy.serverFd_;
		serverPort_ = copy.serverPort_;
		res_ = copy.res_;
	}
	std::cout << "ServerMain's copy was created with the copy assignment operator" << std::endl;
	return (*this);
}

ServerMain::~ServerMain() {
	if (res_)
		freeaddrinfo(res_);
	if (serverFd_ >= 0)
		close(serverFd_);
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
	}
	
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
		if (setsockopt(serverFd_, IPPROTO_TCP, SO_REUSEADDR, &yes, sizeof(yes)) == -1) { // TODO should we use SOL_SOCKET for APIs?
			std::cerr << RED << "setsockopt() failed: " << strerror(errno) << RESET << std::endl;
			close (serverFd_);
			continue;
		}
		if (bind(serverFd_, iterationPointer->ai_addr, iterationPointer->ai_addrlen) == 0) {
			break ; //bind worked!
		}
		close(serverFd_);
	}

	if (iterationPointer == NULL) {
		std::cerr << RED << "Socket() failed: " << strerror(errno) << RESET << std::endl;
		//TODO return? how to stop the program - no exit() allowed
	}

	if (listen(serverFd_, 10) == -1) {
		std::cerr << RED << "listen() failed: " << strerror(errno) << RESET << std::endl;
		//TODO return? how to stop the program - no exit() allowed
	}
	std::cout << "listening on port " << serverPort_ << std::endl;
}