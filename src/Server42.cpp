/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   Server42.cpp                                       :+:    :+:            */
/*                                                     +:+                    */
/*   By: kziari <kziari@student.42.fr>                +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/05/13 10:30:48 by mstencel      #+#    #+#                 */
/*   Updated: 2025/08/18 16:05:41 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../incl/Server42.hpp"

//basic canonical form

Server42::Server42() {
	// std::cout << "Server42 was created" << std::endl;
}

Server42::Server42(const Server42& copy) {
	servers_ = copy.servers_;
	// std::cout << "Server42's copy was created" << std::endl;
}

Server42& Server42::operator=(const Server42& copy) {
	if (this != &copy) {
		servers_ = copy.servers_;
	}
	// std::cout << "Server42's copy was created with the copy assignment operator" << std::endl;
	return (*this);
}

Server42::~Server42() {
	// std::cout << "Server42 was destroyed" << std::endl;
}

//getters:

// Update getter implementation
const std::vector<std::shared_ptr<SingleServer>>& Server42::getServers() const {
    return servers_;
}

//setters:

void Server42::addServer(const std::shared_ptr<SingleServer> newServer) {
    servers_.push_back(newServer);

    std::cout << "Logging first location of each server in servers_:" << std::endl;
    for (size_t i = 0; i < servers_.size(); ++i) {
        const std::vector<std::shared_ptr<Location>>& serverLocs = servers_[i]->getLocations();
        if (!serverLocs.empty() && serverLocs[0]) {
            std::cout << "USE_COUNT  Server[" << i << "] -> Location[0]: " << serverLocs[0].use_count() << std::endl;
            std::cout << "  Server[" << i << "] -> Location[0]: " << serverLocs[0]->getPath() << std::endl;
        } else {
            std::cerr << "  Server[" << i << "] -> No valid first location!" << std::endl;
        }
    }
}