/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   OldConfigParser.cpp                                :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/05/20 12:43:25 by mstencel      #+#    #+#                 */
/*   Updated: 2025/06/12 12:18:16 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../incl/OldConfigParser.hpp"

ConfigParser::ConfigParser(): tokeniser_(nullptr) {
	std::cout << "ConfigParser default constructor called" << std::endl;
}

ConfigParser::ConfigParser(std::stringstream& buffer) {
	tokeniser_.setAllConfig(buffer.str());
	// std::cout << "config: " << tokeniser_.getAllConfig() << std::endl;
	tokeniser_.setCurrentPos(tokeniser_.getAllConfig().begin());
	// currentToken_ = tokeniser_.defineToken();
	// allTokens_.push_back(currentToken_);
	std::cout << "ConfigParser constructor with param called" << std::endl;
}

ConfigParser::ConfigParser(const ConfigParser& copy) {
	tokeniser_ = copy.tokeniser_;
	// currentToken_ = copy.currentToken_;
	// allTokens_ = copy.allTokens_;
	std::cout << "ConfigParser copy constructor called" << std::endl;
}

ConfigParser&	ConfigParser::operator=(const ConfigParser& copy) {
	if (this != &copy) {
		tokeniser_ = copy.tokeniser_;
		// currentToken_ = copy.currentToken_;
		// allTokens_ = copy.allTokens_;
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

// cToken	ConfigParser::getCurrentToken() const {
// 	return (currentToken_);
// }

void	ConfigParser::setTokeniser(const ConfTokeniser& tokeniser) {
	tokeniser_ = tokeniser;
}

// void	ConfigParser::setCurrentToken(const cToken& token) {
// 	currentToken_ = token;
// }

int	ConfigParser::configError(const std::string& errorMessage) const {
	std::cerr << "Error: " << errorMessage << std::endl;
	return (EXIT_FAILURE);
}

int	ConfigParser::parseServerBlock(std::vector<SingleServer>& servers) {
	std::cout << "Parsing server block" << std::endl;
	SingleServer	newServer;
	cToken			currentToken;
	
	currentToken = tokeniser_.defineToken();
	if (currentToken.type != OPEN_BRACE) {
		return (configError("expected '{' after 'server' keyword"));
	}
	else {
		currentToken = tokeniser_.defineToken();
		if (currentToken.type == STRING) {
			if (currentToken.value == "listen") {
				currentToken = tokeniser_.defineToken();
				if (currentToken.type != NUMBER) {
					return (configError("expected an IP address"));
				}
				//TODO check if the IP address is correct
				newServer.setServIP(currentToken.value);
				currentToken = tokeniser_.defineToken();
				if (currentToken.type != SEMICOLON) {
					return (configError("expected ':' after the IP address"));
				}
				currentToken = tokeniser_.defineToken();
				if (currentToken.type != NUMBER) {
					return (configError("expected a port number"));
				}
				//TODO check if the port number is correct
				newServer.setServPortInt(std::stoi(currentToken.value));
			}
			currentToken = tokeniser_.defineToken();
			if (currentToken.value == "server_name") {
				std::cout << "going to fill in the server's name" << std::endl;
				std::vector<std::string> serverNames;
				currentToken = tokeniser_.defineToken();
				if (currentToken.type != STRING) {
					return (configError("expected a server's name"));
				}
				while (currentToken.type == STRING) {
					serverNames.push_back(currentToken.value);
					currentToken = tokeniser_.defineToken();
					if (currentToken.type == SEMICOLON) {
						break ;
					}
					else if (currentToken.type != STRING) {
						return (configError("expected a server's name or ';' at the end of the line"));
					}
				}
			}
			if (currentToken.value == "location") {
				std::cout << "going to parse the location part" << std::endl;
				//fill in the location block
			}
			if (currentToken.type == CLOSE_BRACE) {
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

	cToken	currentToken;
	currentToken = tokeniser_.defineToken();
	while (currentToken.type != END_OF_FILE) {
		if (currentToken.type == STRING && currentToken.value == "server") {
			if (parseServerBlock(servers) != EXIT_SUCCESS) {
				return (EXIT_FAILURE);
			}
		}
		else {
			return (configError("expected 'server' keyword"));
		}
		currentToken = tokeniser_.defineToken();
	}
	return (EXIT_SUCCESS);
}