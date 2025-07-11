/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerMain.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kziari <kziari@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/13 13:30:12 by mstencel          #+#    #+#             */
/*   Updated: 2025/07/08 14:48:34 by kziari           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVERMAIN_HPP
# define SERVERMAIN_HPP

#include <iostream>
#include <netdb.h> //for freeaddrinfo()
#include <unistd.h> //for close()
#include <cstring> //for memset()
#include <string> //for to_string()
#include <fcntl.h> //for fcntl()
#include <sys/epoll.h> //for epoll
#include <map>         //for map
#include "Webserv42.hpp"

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define MAX_EVENTS 64
#define BUFFER_SIZE 30000

class Request;

class ServerMain {
	
	public:
		ServerMain();
		ServerMain(int port);
		ServerMain(const ServerMain& copy);
		ServerMain& operator=(const ServerMain& copy);
		~ServerMain();

		//getters:
		int			getServFd() const;
		int			getServPort() const;
		addrinfo	*getResults() const;

		//setters:
		void	setServFd(const int newFd);
		void	setServPort(const int newPort);
		void	setResults(addrinfo *newResult);

		
		void	startSocket();
		void	startConnection();
		void	eventLoop();

		//epoll
		void setUpEpoll();
		void addFdToEpoll(int fd, uint32_t events);
		void removeFdFromEpoll(int fd);

		//reading and parsing for a client
		void handleClientRead(int clientFd);
	
		// writing and sending responses
		void handleClientWrite(int clientFd);
		
	private:
		int							serverFd_;
		int							serverPort_;
		struct addrinfo				*res_;
		// int							clientFd_;
		// std::string					rawRequest_;
		int							epollFd_;
		std::vector<epoll_event>	events_;
		std::map<int, Request>		clients_requests_;

};

# endif
