/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   Server42.cpp                                       :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/05/13 10:30:48 by mstencel      #+#    #+#                 */
/*   Updated: 2025/05/13 13:49:20 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "incl/Server42.hpp"

//basic canonical form

Server42::Server42():
	// serverPort_(0),
	// serverFd_(-1),
	serverHost_(""),
	serverRoot_(""),
	serverName_(0),
	errorPage_(0)
{
	std::cout << "Server42 was created" << std::endl;
}

Server42::Server42(const Server42& copy):
	// serverPort_(copy.serverPort_),
	// serverFd_(copy.serverFd_),
	serverHost_(copy.serverHost_),
	serverRoot_(copy.serverRoot_),
	serverName_(copy.serverName_),
	errorPage_(copy.errorPage_) {
	std::cout << "Server42's copy was created" << std::endl;
}

Server42& Server42::operator=(const Server42& copy) {
	if (this != &copy) {
		// serverPort_ = copy.serverPort_;
		// serverFd_ = copy.serverFd_;
		serverHost_ = copy.serverHost_;
		serverRoot_ = copy.serverRoot_;
		serverName_ = copy.serverName_;
		errorPage_ = copy.errorPage_;
	}
	std::cout << "Server42's copy was created with the copy assignment operator" << std::endl;
	return (*this);
}

Server42::~Server42() {
	std::cout << "Server42 was destroyed" << std::endl;
}

//getters:

std::string Server42::getServHost() const {
	return (serverHost_);
}

std::string Server42::getServRoot() const {
	return (serverRoot_);
}

std::vector<std::string> Server42::getServName() const {
	return (serverName_);
}

const std::unordered_map<int, std::string>& Server42::getErrorPages() const {
	return (errorPage_);
}

//setters:



void	Server42::setServHost(const std::string &newServHost) {
	serverHost_ = newServHost;
}

void	Server42::setServRoot(const std::string &newServRoot) {
	serverRoot_ = newServRoot;
}

void	Server42::setServName(const std::vector<std::string> &newServName) {
	serverName_ = newServName;
}

void	Server42::setErrorPages(const int errorNb, const std::string &newErrorPage) {
	errorPage_[errorNb] = newErrorPage;
}
