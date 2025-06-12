/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ConfParser.cpp                                     :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/06/12 11:27:51 by mstencel      #+#    #+#                 */
/*   Updated: 2025/06/12 14:21:17 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../incl/ConfParser.hpp"

ConfParser::ConfParser(): currentLine_(1) {
	std::cout << "ConfParser default constructor called" << std::endl;
}

ConfParser::ConfParser(const std::string& config):
	allConfig_(config),
	currentPos_(allConfig_.begin()),
	currentLine_(1) {
		std::cout << "ConfParser constructor with param called" << std::endl;
		std::cout << allConfig_ << std::endl;
		std::cout << "Current position: " << *currentPos_ << std::endl;
}

ConfParser::ConfParser(const ConfParser& copy):
	allConfig_(copy.allConfig_),
	currentPos_(copy.currentPos_),
	currentLine_(copy.currentLine_),
	allTokens_(copy.allTokens_) {
		std::cout << "ConfParser copy constructor called" << std::endl;
}

ConfParser& ConfParser::operator=(const ConfParser& copy) {
	if (this != &copy) {
		allConfig_ = copy.allConfig_;
		currentPos_ = copy.currentPos_;
		currentLine_ = copy.currentLine_;
		allTokens_ = copy.allTokens_;
	}
	std::cout << "ConfParser copy assignment operator called" << std::endl;
	return (*this);
}

ConfParser::~ConfParser() {
	std::cout << "ConfParser default destructor called" << std::endl;
}

int	ConfParser::getCurrentLine() const {
	return (currentLine_);
}

std::vector<cToken> ConfParser::getAllTokens() const {
	return (allTokens_);
}

void	ConfParser::setAllConfig(const std::string& config) {
	allConfig_ = config;
	currentPos_ = allConfig_.begin();
	currentLine_ = 1;
}

void	ConfParser::skipWhiteSpaceComment() {
	for (; currentPos_ != allConfig_.end(); currentPos_++) {
		if (isspace(*currentPos_))
			continue;
		else if (*currentPos_ == '#') {
			while (currentPos_ != allConfig_.end() && *currentPos_ != '\n')
				*currentPos_++;
		}
		else
			break;
	}
}

cToken	ConfParser::defineToken() {
	std::string	tValue;
	while (currentPos_ != allConfig_.end()) {
		skipWhiteSpaceComment();
		if (currentPos_ == allConfig_.end()) {
			std::cout << "EOF found" << std::endl;
			return (cToken(END_OF_FILE, ""));
		}
		else if (*currentPos_ == '{') {
			currentPos_++;
			std::cout << "{ found" << std::endl;
			return (cToken(OPEN_BRACE, "{"));
		}
		else if (*currentPos_ == '}') {
			currentPos_++;
			std::cout << "} found" << std::endl;
			return (cToken(CLOSE_BRACE, "}"));
		}
		else if (*currentPos_ == ':') {
			currentPos_++;
			std::cout << ": found" << std::endl;
			return (cToken(COLON, ":"));
		}
		else if (*currentPos_ == ';') {
			currentPos_++;
			std::cout << "; found" << std::endl;
			return (cToken(SEMICOLON, ";"));
		}
		else if (*currentPos_ == '/') {
			currentPos_++;
			std::cout << "/ found" << std::endl;
			return (cToken(SLASH, "/"));
		}
		else if (isdigit(*currentPos_)) {
			while (currentPos_ != allConfig_.end() && *currentPos_ != '\n' && (isdigit(*currentPos_) || *currentPos_ == '.')) {
				tValue += *currentPos_;
				currentPos_++;			}
			std::cout << tValue << " found" << std::endl;
			return (cToken(NUMBER, tValue));
		}
		else if (isalpha(*currentPos_)) {
			while (currentPos_ != allConfig_.end() && *currentPos_ != '\n' && (isalpha(*currentPos_) || isdigit(*currentPos_) || *currentPos_ == '-' || *currentPos_ == '_' || *currentPos_ == '.')) {
				tValue.push_back(*currentPos_);
				currentPos_++;
			}
			std::cout << tValue << " found" << std::endl;
			return (cToken(STRING, tValue));
		}
		else if (*currentPos_ == '\n') {
			currentLine_++;
			currentPos_++;
			std::cout << "\\n found" << std::endl;
		}
		else {
			while (currentPos_ != allConfig_.end() && *currentPos_ != '\n') {
				tValue += *currentPos_;
				currentPos_++;
	}
	std::cout << "Unknown input: " << tValue << " found" << std::endl;
	return (cToken(UNKNOWN, tValue));
		}
	}
	if (currentPos_ == allConfig_.end()) {
		std::cout << "EOF found" << std::endl;
		return (cToken(END_OF_FILE, ""));
	}
	return (cToken(UNKNOWN, ""));
}

int	ConfParser::setAllTokens() {
	
	cToken	currentToken;
	while (currentPos_ < allConfig_.end()) {
		currentToken = defineToken();
		if (currentToken.type == UNKNOWN) {
			std::cerr << "Error: Unknown token '" << currentToken.value << "' at line " << currentLine_ << std::endl;
			return (EXIT_FAILURE);
		}
		allTokens_.push_back(currentToken);
	}
	for (size_t i = 0; i < allTokens_.size(); i++) {
		std::cout << "Token: " << allTokens_[i].type << "\tValue: " << allTokens_[i].value << std::endl;
	}
	return (EXIT_SUCCESS);
}

int	ConfParser::populateServers(Server42& servers) {
	
	for (size_t i = 0; i < allTokens_.size(); i++) {
		if (allTokens_[i].type == STRING && allTokens_[i].value == "server") {
			SingleServer	newServer;
			servers.addServer(newServer);
			//need to populate the server block - pass the i to the function
		}
	}
	return (EXIT_SUCCESS);
}

int ConfParser::parseConfig(Server42& servers) {
	
	if (setAllTokens() == EXIT_FAILURE) {
		std::cerr << "Error following up - from setAllTokens() in parseConfig()" << std::endl;
		return (EXIT_FAILURE);
	}
	if (populateServers(servers) == EXIT_FAILURE) {
		std::cerr << "Error following up - from populateServers() in parseConfig()" << std::endl;
		return (EXIT_FAILURE);
	}
	
	return (EXIT_SUCCESS);
}