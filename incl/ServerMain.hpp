/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ServerMain.hpp                                     :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/05/13 13:30:12 by mstencel      #+#    #+#                 */
/*   Updated: 2025/05/13 15:05:21 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVERMAIN_HPP
# define SERVERMAIN_HPP

#include <iostream>
#include <netdb.h> //for freeaddrinfo()
#include <unistd.h> //for close()
#include <cstring> //for memset()
#include <string> //for to_string()

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"

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
	
	private:
		int				serverFd_;
		int				serverPort_;
		struct addrinfo	*res_;
		

};

# endif
