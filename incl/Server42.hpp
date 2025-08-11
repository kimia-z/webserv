/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   Server42.hpp                                       :+:    :+:            */
/*                                                     +:+                    */
/*   By: kziari <kziari@student.42.fr>                +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/05/05 10:37:22 by mstencel      #+#    #+#                 */
/*   Updated: 2025/08/11 13:22:50 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER42
# define SERVER42

#include "Webserv42.hpp"

#include <vector>
#include <unordered_map>
#include <iostream>
#include <memory> // for std::shared_ptr

class SingleServer;

class Server42 {

	public:
		Server42();
		Server42(const Server42& copy);
		Server42& operator=(const Server42& copy);
		~Server42();

		// getters:
		const std::vector<std::shared_ptr<SingleServer>>&	getServers() const;
		
		
		//setters:
		void	addServer(const std::shared_ptr<SingleServer> newServer);

		
	
	private:
		std::vector<std::shared_ptr<SingleServer>>	servers_;
};

#endif