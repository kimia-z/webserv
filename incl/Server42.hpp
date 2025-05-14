/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   Server42.hpp                                       :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/05/05 10:37:22 by mstencel      #+#    #+#                 */
/*   Updated: 2025/05/13 13:49:07 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER42
# define SERVER42

#include "Webserv42.hpp"

#include <vector>
#include <unordered_map>
#include <iostream>

class Server42 {

	public:
		Server42();
		Server42(const Server42& copy);
		Server42& operator=(const Server42& copy);
		~Server42();

		// getters:
		// int											getServPort() const;
		// int											getServFd() const;
		std::string									getServHost() const;
		std::string									getServRoot() const;
		std::vector<std::string>					getServName() const;
		const std::unordered_map<int, std::string>	&getErrorPages() const; //using "const &"" as a return value is faster, cause the map doesn't need to be copied
		
		//setters:
		// void	setServPort(int newServPort);
		// void	setServFd(int newServFd);
		void	setServHost(const std::string &newServHost);
		void	setServRoot(const std::string &newServRoot);
		void	setServName(const std::vector<std::string> &newServName);
		void	setErrorPages(const int errorNb, const std::string &newErrorPage);

		
	
	private:
		// int										serverPort_;
		// int										serverFd_;
		std::string								serverHost_;
		std::string								serverRoot_;
		std::vector<std::string>				serverName_;
		std::unordered_map<int, std::string>	errorPage_;
		
		
		
};

#endif