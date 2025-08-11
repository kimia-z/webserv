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

SingleServer::SingleServer():
	serverName_(""),
	locations_(),
	serverRoot_(""),
	serverIP_(""),
	serverPortString_("8081"),
	serverPortInt_(-1),
	serverFd_(-1),
	maxBodySize_(-1),
	errorPages_(),
	res_(nullptr)
{}

SingleServer::SingleServer(int port):
	serverName_(""),
	locations_(),
	serverRoot_(""),
	serverIP_(""),
	serverPortString_(std::to_string(port)),
	serverPortInt_(port),
	serverFd_(-1),
	maxBodySize_(-1),
	errorPages_(),
	res_(nullptr)
{}

SingleServer::SingleServer(const SingleServer& copy):
	serverName_(copy.serverName_),
	locations_(copy.locations_),
	serverRoot_(copy.serverRoot_),
	serverIP_(copy.serverIP_),
	serverPortString_(copy.serverPortString_),
	serverPortInt_(copy.serverPortInt_),
	serverFd_(copy.serverFd_),
	maxBodySize_(copy.maxBodySize_),
	errorPages_(copy.errorPages_),
	res_(copy.res_)
{}

SingleServer&	SingleServer::operator=(const SingleServer& copy) {
	if (this != &copy) {
		if (serverFd_ >= 0) { // If serverFd_ is active, close it first before assigning new one.
			close(serverFd_);
			serverFd_ = -1;
		}
		serverName_ = copy.serverName_;
		locations_ = copy.locations_;
		serverRoot_ = copy.serverRoot_;
		serverIP_ = copy.serverIP_;
		serverPortString_ = copy.serverPortString_;
		serverPortInt_ = copy.serverPortInt_;
		serverFd_ = copy.serverFd_;
		maxBodySize_ = copy.maxBodySize_;
		errorPages_ = copy.errorPages_;
		res_ = copy.res_;
	}
	return (*this);
}

SingleServer::~SingleServer() {
	if (serverFd_ >= 0)
		close(serverFd_);
}

// getters
std::string SingleServer::getServName() const { return (serverName_); }
const std::vector<std::shared_ptr<Location>>& SingleServer::getLocations() const { return (locations_); }
std::string  SingleServer::getServRoot() const { return (serverRoot_); }
std::string SingleServer::getServIP() const { return (serverIP_); }
std::string SingleServer::getServPortString() const { return (serverPortString_); }
int SingleServer::getServPortInt() const { return (serverPortInt_); }
int SingleServer::getServFd() const { return (serverFd_); }
int SingleServer::getMaxBodySize() const { return (maxBodySize_); }
const std::unordered_map<int, std::string>& SingleServer::getErrorPages() const { return (errorPages_); }
addrinfo *SingleServer::getResults() const { return res_.get(); }

// setters
void    SingleServer::setServName(const std::string& newServName) { serverName_ = newServName; }
void    SingleServer::setLocations(const std::shared_ptr<Location>& newLocation) { locations_.push_back(newLocation); }
void    SingleServer::setServRoot(const std::string& newServRoot) { serverRoot_ = newServRoot; }
void    SingleServer::setServIP(const std::string& newServIP) { serverIP_ = newServIP; }
void    SingleServer::setServPortString(const std::string& newServPortStr) { serverPortString_ = newServPortStr; }
void    SingleServer::setServPortInt(const int& newServPortInt) { serverPortInt_ = newServPortInt; }
void    SingleServer::setServFd(const int& newServFd) { serverFd_ = newServFd; }
void    SingleServer::setMaxBodySize(const int& newMaxBodySize) { maxBodySize_ = newMaxBodySize; }
void    SingleServer::setErrorPages(const int& errorNb, const std::string& newErrorPage) { errorPages_[errorNb] = newErrorPage; }
void    SingleServer::setResults(addrinfo* newResult) { res_ = std::shared_ptr<addrinfo>(newResult, freeaddrinfo);
}

std::string SingleServer::getErrorPagePath(int errorCode) const {
	auto it = errorPages_.find(errorCode);
	if (it != errorPages_.end()) {
		return it->second;
	}
	return "";
}


void	SingleServer::initSocket()
{
	struct addrinfo	hints;
	struct addrinfo	*iterationPointer;
	int				status;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		//works for both IPv4 & IPv6
	hints.ai_socktype = SOCK_STREAM;	//for the TCP
	hints.ai_flags = AI_PASSIVE;		// fills it in with the localhost address (0.0.0.0 or ::)

	addrinfo* raw_res = nullptr;
	if ((status = getaddrinfo(serverIP_.c_str(), serverPortString_.c_str(), &hints, &raw_res)) != 0) {
		throw std::runtime_error("Failed to get address info for server socket.");
	}
	res_ = std::shared_ptr<addrinfo>(raw_res, freeaddrinfo);

	for (iterationPointer = res_.get(); iterationPointer != NULL; iterationPointer = iterationPointer->ai_next) {
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
		if (bind(serverFd_, iterationPointer->ai_addr, iterationPointer->ai_addrlen) == 0) {
			break ; // Bind successful
		}
		std::cerr << RED << "Bind() failed for port " << serverPortString_ << ": " << strerror(errno) << RESET << std::endl;
		close(serverFd_); // Close socket if bind failed, try next address
	}

	if (iterationPointer == NULL) { // If loop finished without successful bind
		throw std::runtime_error("Failed to bind server socket to any address.");
	}
	if (listen(serverFd_, 10) == -1) {
		close(serverFd_);
		throw std::runtime_error("Failed to listen on server socket.");
	}
}