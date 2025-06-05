/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   Server42.cpp                                       :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/05/13 10:30:48 by mstencel      #+#    #+#                 */
/*   Updated: 2025/05/16 13:10:50 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../incl/Server42.hpp"

//basic canonical form

Server42::Server42():
	serverHost_(""),
	serverRoot_(""),
	serverName_(0),
	errorPage_(0)
{
	std::cout << "Server42 was created" << std::endl;
}

Server42::Server42(const Server42& copy):
	serverHost_(copy.serverHost_),
	serverRoot_(copy.serverRoot_),
	serverName_(copy.serverName_),
	errorPage_(copy.errorPage_) {
	std::cout << "Server42's copy was created" << std::endl;
}

Server42& Server42::operator=(const Server42& copy) {
	if (this != &copy) {
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


//setters:
