/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kziari <kziari@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/05 10:43:12 by mstencel          #+#    #+#             */
/*   Updated: 2025/07/11 14:09:03 by kziari           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incl/Webserv42.hpp"

/// @brief opens the conFile and saves it in conFile, 
	// checks if the conf file can be opened, if it's empty
/// @param path 
/// @return 
void	openConfFile(const char *path, std::ifstream& conFile) {

	conFile.open(path, std::ifstream::in);
	if (!conFile.is_open()) {
		throw std::runtime_error("conf file cannot be opened");
	}
	if (conFile.peek() == std::ifstream::traits_type::eof()) {
		conFile.close();
		throw std::runtime_error("configuration file is empty");
	}
}

int	main(int argc, char **argv)
{
	if (argc > 2) {
		std::cerr << "Error: wrong program usage. Type ./webserv [configuration file]" << std::endl;
		return (-1);
	}
	std::ifstream	confFile;
	try {
		openConfFile(argc == 2 ? argv[1] : "webserv.conf", confFile);
		
		std::stringstream	buffer;
		buffer << confFile.rdbuf();
		confFile.close();
		
		ConfParser	conf(buffer.str());
		Server42	servers;
		
		conf.parseConfig(servers);
		for (SingleServer& server : servers.getServers()) {
			server.initSocket();
			server.acceptConnections();
			// server.setUpEpoll();
			// server.eventLoop();
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Unexpected error: " << e.what() << std::endl;
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}
