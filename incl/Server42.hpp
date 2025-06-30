/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   Server42.hpp                                       :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/05/05 10:37:22 by mstencel      #+#    #+#                 */
/*   Updated: 2025/06/11 11:21:36 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER42
# define SERVER42

#include "Webserv42.hpp"

#include <vector>
#include <unordered_map>
#include <iostream>

class SingleServer;

class Server42 {

	public:
		Server42();
		Server42(const Server42& copy);
		Server42& operator=(const Server42& copy);
		~Server42();

		// getters:
		std::vector<SingleServer>	getServers() const;
		
		
		//setters:
		void	addServer(const SingleServer& newServer);

		
	
	private:
		std::vector<SingleServer>	servers_;	
};

#endif