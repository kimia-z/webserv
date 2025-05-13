/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   main.cpp                                           :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/05/13 10:37:48 by mstencel      #+#    #+#                 */
/*   Updated: 2025/05/13 14:14:16 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "incl/Webserv42.hpp"

int	main(int argc, char *argv[])
{
	if (argc > 2) {
		std::cerr << "Error: wrong program usage. Type ./webserv [configuration file]" << std::endl;
		return (-1);
	}
	ServerMain	server;
	//parse the config file and populate the server

	server.startSocket();
	server.startConnection();
	return (0);
}