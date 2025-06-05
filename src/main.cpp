/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   main.cpp                                           :+:    :+:            */
/*                                                     +:+                    */
/*   By: kziari <kziari@student.42.fr>                +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/05/13 10:37:48 by mstencel      #+#    #+#                 */
/*   Updated: 2025/05/26 10:56:08 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../incl/Webserv42.hpp"

/// @brief checks if the conf file can be opened, if it's empty and 
/// @param conFile 
/// @return 
std::ifstream	openConfFile(const char *path) {

	std::ifstream	conFile(path, std::ifstream::in);
	if (!conFile.is_open()) {
		throw std::runtime_error("Error: conf file cannot be opened");
	}
	if (conFile.peek() == std::ifstream::traits_type::eof()) {
		conFile.close();
		throw std::runtime_error("Error: configuration file is empty");
	}
	return (conFile);
}

int	main(int argc, char **argv)
{
	if (argc > 2) {
		std::cerr << "Error: wrong program usage. Type ./webserv [configuration file]" << std::endl;
		return (-1);
	}
	std::ifstream	confFile;
	try {
		confFile = argc == 2 ? openConfFile(argv[1]) : openConfFile("webserv.conf");
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return (-1);
	}
	std::stringstream	buffer;
	buffer << confFile.rdbuf();
	confFile.close();
	std::cout << "test\n" << buffer.str() << std::endl;

	ConfigParser	confParser(buffer);
	std::vector<SingleServer> servers;
	if (confParser.parseConfig(servers) != EXIT_SUCCESS) {
		std::cerr << "Error: configuration file parsing failed" << std::endl;
		return (-1);
	}
	//parse the config file and populate the server
	
	SingleServer	server;

	server.initSocket();
	server.acceptConnections();
	return (0);
}