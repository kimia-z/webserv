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
	// serverNames_(),
	// serverHost_(""),
	locations_(),
	serverRoot_(""),
	serverIP_(""),
	serverPortString_("8081"),
	serverPortInt_(-1),
	serverFd_(-1),
	maxBodySize_(-1),
	errorPages_(),
	res_(nullptr)
	{
	// std::cout << "SingleServer was constructed" << std::endl;
}

SingleServer::SingleServer(int port):
	serverName_(""),
	// serverNames_(),
	// serverHost_(""),
	locations_(),
	serverRoot_(""),
	serverIP_(""),
	serverPortString_(std::to_string(port)),
	serverPortInt_(port),
	serverFd_(-1),
	maxBodySize_(-1),
	errorPages_(),
	res_(nullptr)
	{
	// std::cout << "SingleServer with port: " << port << " was constructed" <<std::endl;
}

SingleServer::SingleServer(const SingleServer& copy):
	serverName_(copy.serverName_),
	// serverNames_(copy.serverNames_),
	// serverHost_(copy.serverHost_),
	locations_(copy.locations_),
	serverRoot_(copy.serverRoot_),
	serverIP_(copy.serverIP_),
	serverPortString_(copy.serverPortString_),
	serverPortInt_(copy.serverPortInt_),
	serverFd_(copy.serverFd_),
	maxBodySize_(copy.maxBodySize_),
	errorPages_(copy.errorPages_),
	res_(nullptr)
	{
}

SingleServer&	SingleServer::operator=(const SingleServer& copy) {
	if (this != &copy) {
		// If serverFd_ is active, close it first before assigning new one.
		if (serverFd_ >= 0) {
			close(serverFd_);
			serverFd_ = -1;
		}
		if (res_) {
			freeaddrinfo(res_);
			res_ = nullptr;
		}

		serverName_ = copy.serverName_;
		// serverNames_ = copy.serverNames_;
		locations_ = copy.locations_;
		serverRoot_ = copy.serverRoot_;
		serverIP_ = copy.serverIP_;
		serverPortString_ = copy.serverPortString_;
		serverPortInt_ = copy.serverPortInt_;
		serverFd_ = copy.serverFd_; // FD copied
		maxBodySize_ = copy.maxBodySize_;
		errorPages_ = copy.errorPages_;
		res_ = copy.res_; // Pointer copied, careful with double free
	}
	// std::cout << "SingleServer's copy was created with the copy assignment operator" << std::endl;
	return (*this);
}

SingleServer::~SingleServer() {
	// Only free res_ if this instance owns it (not a copied pointer)
	if (res_)
		freeaddrinfo(res_);
	
	// Only close serverFd_ if this instance created it and owns it.
	// If you allowed shallow copy, this is dangerous.
	// It's safer if serverFd_ is only ever closed by the original creator.
	if (serverFd_ >= 0)
		close(serverFd_);
	
	// std::cout << "SingleServer was destructed" << std::endl;
}

// getters
std::string SingleServer::getServName() const { return (serverName_); }
// const std::vector<std::string>& SingleServer::getServerNames() const { return (serverNames_); }
const std::vector<Location>& SingleServer::getLocations() const { return (locations_); }
std::string  SingleServer::getServRoot() const { return (serverRoot_); }
std::string SingleServer::getServIP() const { return (serverIP_); }
std::string SingleServer::getServPortString() const { return (serverPortString_); }
int SingleServer::getServPortInt() const { return (serverPortInt_); }
int SingleServer::getServFd() const { return (serverFd_); }
int SingleServer::getMaxBodySize() const { return (maxBodySize_); }
const std::unordered_map<int, std::string>& SingleServer::getErrorPages() const { return (errorPages_); }
addrinfo    *SingleServer::getResults() const { return (res_); }

// Helper to get custom error page path
std::string SingleServer::getErrorPagePath(int errorCode) const {
	auto it = errorPages_.find(errorCode);
	if (it != errorPages_.end()) {
		return it->second; // Return custom path if found
	}
	return ""; // Return empty string if no custom page is configured
}

// setters
void    SingleServer::setServName(const std::string& newServName) { serverName_ = newServName; }
// void    SingleServer::addServerName(const std::string& newName) { serverNames_.push_back(newName); }
void    SingleServer::setLocations(const Location& newLocation) { locations_.push_back(newLocation); }
void    SingleServer::setServRoot(const std::string& newServRoot) { serverRoot_ = newServRoot; }
void    SingleServer::setServIP(const std::string& newServIP) { serverIP_ = newServIP; }
void    SingleServer::setServPortString(const std::string& newServPortStr) { serverPortString_ = newServPortStr; }
void    SingleServer::setServPortInt(const int& newServPortInt) { serverPortInt_ = newServPortInt; }
void    SingleServer::setServFd(const int& newServFd) { serverFd_ = newServFd; }
void    SingleServer::setMaxBodySize(const int& newMaxBodySize) { maxBodySize_ = newMaxBodySize; }
void    SingleServer::setErrorPages(const int& errorNb, const std::string& newErrorPage) { errorPages_[errorNb] = newErrorPage; }
void    SingleServer::setResults(addrinfo* newResult) {
	if (res_) freeaddrinfo(res_);
	res_ = newResult;
}

// Socket Initialization (sets up listening FD, makes it non-blocking)
void	SingleServer::initSocket() {
	struct addrinfo hints;
	struct addrinfo *iterationPointer;
	int				status;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; //works for both IPv4 & IPv6
	hints.ai_socktype = SOCK_STREAM; //for the TCP
	hints.ai_flags = AI_PASSIVE; // fills it in with the localhost address (0.0.0.0 or ::)

	if ((status = getaddrinfo(serverIP_.c_str(), serverPortString_.c_str(), &hints, &res_)) != 0) {
		std::cerr << RED << "getaddrinfo error for port " << serverPortString_ << ": " << gai_strerror(status) << RESET << std::endl;
		throw std::runtime_error("Failed to get address info for server socket.");
	}
	for (iterationPointer = res_; iterationPointer != NULL; iterationPointer = iterationPointer->ai_next) {
		serverFd_ = socket(iterationPointer->ai_family, iterationPointer->ai_socktype, iterationPointer->ai_protocol);
		if (serverFd_ == -1) {
			std::cerr << RED << "Socket() failed for port " << serverPortString_ << ": " << strerror(errno) << RESET << std::endl;
			continue;
		}
		int yes = 1;
		if (setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
			std::cerr << RED << "setsockopt(SO_REUSEADDR) failed for port " << serverPortString_ << ": " << strerror(errno) << RESET << std::endl;
			close (serverFd_);
			continue;
		}
		// Bind the socket
		if (bind(serverFd_, iterationPointer->ai_addr, iterationPointer->ai_addrlen) == 0) {
			break ; // Bind successful
		}
		std::cerr << RED << "Bind() failed for port " << serverPortString_ << ": " << strerror(errno) << RESET << std::endl;
		close(serverFd_); // Close socket if bind failed, try next address
	}

	if (iterationPointer == NULL) { // If loop finished without successful bind
		std::cerr << RED << "Could not bind to any address for port " << serverPortString_ << RESET << std::endl;
		throw std::runtime_error("Failed to bind server socket to any address.");
	}
	// Start listening for incoming connections
	if (listen(serverFd_, 10) == -1) {
		std::cerr << RED << "listen() failed for port " << serverPortString_ << ": " << strerror(errno) << RESET << std::endl;
		close(serverFd_);
		throw std::runtime_error("Failed to listen on server socket.");
	}
	// std::cout << "Listening on port " << serverPortString_ << " (FD: " << serverFd_ << ")" << std::endl; // Debug is in Webserv::start()
}