/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server42.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kziari <kziari@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/05 10:37:22 by mstencel          #+#    #+#             */
/*   Updated: 2025/07/25 17:04:35 by kziari           ###   ########.fr       */
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