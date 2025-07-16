/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server42.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kziari <kziari@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/13 10:30:48 by mstencel          #+#    #+#             */
/*   Updated: 2025/07/16 15:03:20 by kziari           ###   ########.fr       */
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

std::vector<SingleServer> Server42::getServers() const {

	return (servers_);
}

//setters:

void	Server42::addServer(const SingleServer& newServer) {
	servers_.push_back(newServer);
}