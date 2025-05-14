/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kziari <kziari@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/13 10:37:48 by mstencel          #+#    #+#             */
/*   Updated: 2025/05/14 14:20:41 by kziari           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "incl/Webserv42.hpp"

int	main()
{
	// if (argc > 2) {
	// 	std::cerr << "Error: wrong program usage. Type ./webserv [configuration file]" << std::endl;
	// 	return (-1);
	// }
	ServerMain	server;
	//parse the config file and populate the server

	server.startSocket();
	server.startConnection();
	return (0);
}