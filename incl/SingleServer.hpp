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

#include "Server42.hpp"

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"

struct	location {
	std::string					path;
	std::string					index;
	std::vector<std::string>	method;
	bool						autoindex;
};

class SingleServer {
	
	public:
		SingleServer();
		SingleServer(int port);
		SingleServer(const SingleServer& copy);
		SingleServer& operator=(const SingleServer& copy);
		~SingleServer();

		//getters:
		std::vector<std::string>					getServName() const;
		// std::string									getServHost() const;
		std::string									getServRoot() const;
		std::string									getServIP() const;
		std::string									getServPortString() const;
		int											getServPortInt() const;
		int											getServFd() const;
		int											getClientFd() const;
		addrinfo									*getResults() const;
		const std::unordered_map<int, std::string>&	getErrorPages() const; //using "const &"" as a return value is faster, cause the map doesn't need to be copied

		//setters:
		void	setServName(const std::vector<std::string> &newServName);
		// void	setServHost(const std::string &newServHost);
		void	setServRoot(const std::string &newServRoot);
		void	setServIP(const std::string& newIP);
		void	setServPortString(const std::string& newPortStr);
		void	setServPortInt(const int newPortInt);
		void	setResults(addrinfo *newResult);
		void	setServFd(const int newServFd);
		void	setClientFd(const int newClientFd);
		void	setErrorPages(const int errorNb, const std::string &newErrorPage);

		
		void	initSocket();
		void	acceptConnections();
	
	private:
		std::vector<std::string>				serverName_; // server's names of the domains
		std::vector<location>					locations_; // server's urls and their locations
		// std::string								serverHost_; // server's host, e.g. "localhost" - within the serverName_, so not needed?
		std::string								serverRoot_; // server's root, e.g. "www/html"
		std::string								serverIP_; //server's IP as a string
		std::string								serverPortString_; //server's port as a string
		int										serverPortInt_; //server's port as an integer
		int										serverFd_; //socket() fd
		int										clientFd_; //accept() fd
		struct addrinfo							*res_; //getaddrinfo() results, needed for bind() & accept()
		std::unordered_map<int, std::string>	errorPage_; //all error pages, key = error number & value = page path
		// std::string								rawRequest_;
};

# endif
