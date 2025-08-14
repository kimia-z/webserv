/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kziari <kziari@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/05 10:43:12 by mstencel          #+#    #+#             */
/*   Updated: 2025/07/25 17:05:03 by kziari           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../incl/Webserv42.hpp"

/// @brief opens the conFile and saves it in conFile, 
	// checks if the conf file can be opened, if it's empty
/// @param path 
/// @return 

// volatile = This prevents the compiler from making assumptions about the variable's value
// sig_atomic_t = a integer type that can be read or written in a single

volatile sig_atomic_t	g_running = 0;

void	signalHandler(int sig)
{
	if (sig == SIGINT)
	{
		std::cerr << "\nSIGINT received. Shutting down gracefully..." << std::endl;
		g_running = 1;
	}
}

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
int main(int argc, char **argv)
{
	// Register the signal handle./we	r for SIGINT (Ctrl+C)
	struct sigaction sa;
	sa.sa_flags = 0;
	sa.sa_handler = signalHandler;	// The sa_mask is a set of signals that should be blocked while handler is running
	sigemptyset(&sa.sa_mask); // ensures no other signals are blocked.
	if (sigaction(SIGINT, &sa, NULL) == -1)
	{
		std::cerr << "Failed to register signal handler" << std::endl;
		return (EXIT_FAILURE);
	}

	if (argc > 2) {
		std::cerr << "Error: wrong program usage. Type ./webserv [configuration file]" << std::endl;
		return (EXIT_FAILURE);
	}

	std::ifstream	confFile;
	Server42		allServerConfigs;

	try {
		openConfFile(argc == 2 ? argv[1] : "webserv.conf", confFile);
		std::stringstream buffer;
		buffer << confFile.rdbuf();
		confFile.close();
		ConfParser parser(buffer.str());
		parser.parseConfig(allServerConfigs);
		Webserv webserver(allServerConfigs); 
		webserver.start();

	} catch (const std::exception& e) {
		std::cerr << RED << "Fatal Error: " << e.what() << RESET << std::endl;
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}