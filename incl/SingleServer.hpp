/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   SingleServer.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kziari <kziari@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/13 13:30:12 by mstencel          #+#    #+#             */
/*   Updated: 2025/05/13 17:15:33 by kziari           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SINGLESERVER_HPP
# define SINGLESERVER_HPP

#include <iostream>
#include <netdb.h> //for freeaddrinfo()
#include <unistd.h> //for close()
#include <cstring> //for memset()
#include <string> //for to_string()
#include <vector> //for std::vector
#include <unordered_map> // for std::unordered_map
#include <sys/epoll.h> //for epoll
#include <map>         //for map

#include "Webserv42.hpp"

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define MAX_EVENTS 64
#define BUFFER_SIZE 30000

class Location;
class Request;

class SingleServer {
private:
	std::string								serverName_; // server's name of the domain
	// std::vector<std::string>                serverNames_; // List of server names
	std::vector<Location>					locations_; // server's urls and their locations
	// std::string							serverHost_; // server's host, e.g. "localhost" - within the serverName_, so not needed?
	std::string								serverRoot_; // server's root, path to the folder with sites, for us "www"
	std::string								serverIP_; //server's IP as a string
	std::string								serverPortString_; //server's port as a string
	int										serverPortInt_;//server's port as an integer
	int										serverFd_;   // The listening socket FD for this server
	int										maxBodySize_; //max size of the uploadable file in bytes
	std::unordered_map<int, std::string>	errorPages_; //all error pages, key = error number & value = page path
	addrinfo								*res_;  //getaddrinfo() results, needed for bind() & accept()
public:
	SingleServer();
	SingleServer(int port);
	SingleServer(const SingleServer& copy);
	SingleServer& operator=(const SingleServer& copy);
	~SingleServer();

	// Getters
	std::string									getServName() const;
	// const std::vector<std::string>& getServerNames() const;
	const std::vector<Location>&				getLocations() const;
	std::string									getServRoot() const;
	std::string									getServIP() const;
	std::string									getServPortString() const;
	int											getServPortInt() const;
	int 										getServFd() const;
	int											getMaxBodySize() const;
	const std::unordered_map<int, std::string>&	getErrorPages() const;
	std::string									getErrorPagePath(int errorCode) const;
	addrinfo									*getResults() const;

	// Setters
	void	setServName(const std::string& newServName);
	// void	addServerName(const std::string& newName); // For multiple server_names
	void	setLocations(const Location& newLocation);
	void	setServRoot(const std::string& newServRoot);
	void	setServIP(const std::string& newServIP);
	void	setServPortString(const std::string& newServPortStr);
	void	setServPortInt(const int& newServPortInt);
	void	setServFd(const int& newServFd);
	void	setMaxBodySize(const int& newMaxBodySize);
	void	setErrorPages(const int& errorNb, const std::string& newErrorPage);
	void	setResults(addrinfo* newResult);

	// Socket Initialization (sets up listening FD, makes it non-blocking)
	void	initSocket();
};

#endif