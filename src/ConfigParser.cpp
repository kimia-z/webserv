/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ConfigParser.cpp                                   :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/05/20 12:43:25 by mstencel      #+#    #+#                 */
/*   Updated: 2025/05/26 13:32:12 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../incl/ConfigParser.hpp"

ConfigParser::ConfigParser(): tokeniser_(nullptr), currentToken_(currentToken_.type = UNKNOWN, currentToken_.value = "") {
	std::cout << "ConfigParser default constructor called" << std::endl;
}

ConfigParser::ConfigParser(std::stringstream& buffer) {
	tokeniser_.setAllConfig(buffer.str());
	currentToken_ = tokeniser_.defineToken();
	std::cout << "ConfigParser constructor with param called" << std::endl;
	
}

ConfigParser::ConfigParser(const ConfigParser& copy) {
	tokeniser_ = copy.tokeniser_;
	currentToken_ = copy.currentToken_;
	std::cout << "ConfigParser copy constructor called" << std::endl;
}

ConfigParser&	ConfigParser::operator=(const ConfigParser& copy) {
	if (this != &copy) {
		tokeniser_ = copy.tokeniser_;
		currentToken_ = copy.currentToken_;
	}
	std::cout << "ConfigParser copy assignment operator called" << std::endl;
	return (*this);
}

ConfigParser::~ConfigParser() {
	std::cout << "ConfigParser default destructor called" << std::endl;
}

ConfTokeniser ConfigParser::getTokeniser() const {
	return (tokeniser_);
}

cToken	ConfigParser::getCurrentToken() const {
	return (currentToken_);
}

void	ConfigParser::setTokeniser(const ConfTokeniser& tokeniser) {
	tokeniser_ = tokeniser;
}

void	ConfigParser::setCurrentToken(const cToken& token) {
	currentToken_ = token;
}

int	ConfigParser::configError(const std::string& errorMessage) const {
	std::cerr << "Error: " << errorMessage << std::endl;
	return (EXIT_FAILURE);
}

int	ConfigParser::parseServerBlock(std::vector<SingleServer>& servers) {
	std::cout << "Parsing server block" << std::endl;
	SingleServer	newServer;
	
	currentToken_ = tokeniser_.defineToken();
	if (currentToken_.type != OPEN_BRACE) {
		return (configError("expected '{' after 'server' keyword"));
	}
	else {
		currentToken_ = tokeniser_.defineToken();
		if (currentToken_.type == STRING) {
			if (currentToken_.value == "listen") {
				currentToken_ = tokeniser_.defineToken();
				if (currentToken_.type != NUMBER) {
					return (configError("expected an IP address"));
				}
				//TODO check if the IP address is correct
				newServer.setServIP(currentToken_.value);
				currentToken_ = tokeniser_.defineToken();
				if (currentToken_.type != SEMICOLON) {
					return (configError("expected ':' after the IP address"));
				}
				currentToken_ = tokeniser_.defineToken();
				if (currentToken_.type != NUMBER) {
					return (configError("expected a port number"));
				}
				//TODO check if the port number is correct
				newServer.setServPort(std::stoi(currentToken_.value));
			}
			currentToken_ = tokeniser_.defineToken();
			if (currentToken_.value == "server_name") {
				std::cout << "going to fill in the server's name" << std::endl;
				newServer.set//fill in the server's name
			}
			if (currentToken_.value == "location") {
				std::cout << "going to parse the location part" << std::endl;
				//fill in the location block
			}
			if (currentToken_.type == CLOSE_BRACE) {
				std::cout << "done parsing the server block" << std::endl;
			}
			else {
				return (configError("expected '}' to close the server block"));
			}
		}
		servers.push_back(newServer);
	}
	return (EXIT_SUCCESS);
}

int	ConfigParser::parseConfig(std::vector<SingleServer>& servers) {
	currentToken_ = tokeniser_.defineToken();
	while (currentToken_.type != END_OF_FILE) {
		if (currentToken_.type == STRING && currentToken_.value == "server") {
			if (parseServerBlock(servers) != EXIT_SUCCESS) {
				return (EXIT_FAILURE);
			}
		}
		else {
			return (configError("expected 'server' keyword"));
		}
		currentToken_ = tokeniser_.defineToken();
	}
	return (EXIT_SUCCESS);
}