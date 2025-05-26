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
#include "../incl/ServerMain.hpp"

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

int	ConfigParser::parseServerBlock() {
	std::cout << "Parsing server block" << std::endl;
	currentToken_ = tokeniser_.defineToken();
	if (currentToken_.type != OPEN_BRACE) {
		return (configError("expected '{' after 'server' keyword"));
	}
	else {
		currentToken_ = tokeniser_.defineToken();
		if (currentToken_.type == STRING) {
			if (currentToken_.value == "listen") {
				//fill up the server's listen port
			}
			if (currentToken_.value == "server_name") {
				//fill in the server's name
			}
			if (currentToken_.value == "location") {
				//fill in the location block
			}
			if (currentToken_.type == CLOSE_BRACE) {
				std::cout << "done parsing the server block" << std::endl;
			}
			else {
				return (configError("expected '}' to close the server block"));
			}
		}
	}
	return (EXIT_SUCCESS);
}

int	ConfigParser::parseConfig() {
	currentToken_ = tokeniser_.defineToken();
	while (currentToken_.type != END_OF_FILE) {
		if (currentToken_.type == STRING && currentToken_.value == "server") {
			parseServerBlock();
		}
		else {
			return (configError("expected 'server' keyword"));
		}
	}
}