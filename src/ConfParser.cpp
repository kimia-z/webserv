/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ConfParser.cpp                                     :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/06/12 11:27:51 by mstencel      #+#    #+#                 */
/*   Updated: 2025/06/18 12:08:38 by mstencel      ########   odam.nl         */
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
	//TODO - if enough time, change it to cases instead of ifs?
	while (currentPos_ != allConfig_.end()) {
		skipWhiteSpaceComment();
		if (currentPos_ == allConfig_.end()) {
			std::cout << "EOF found" << std::endl;
			return (cToken(END_OF_FILE, "", currentLine_));
		}
		else if (*currentPos_ == '{') {
			currentPos_++;
			std::cout << "{ found" << std::endl;
			return (cToken(OPEN_BRACE, "{", currentLine_));
		}
		else if (*currentPos_ == '}') {
			currentPos_++;
			std::cout << "} found" << std::endl;
			return (cToken(CLOSE_BRACE, "}", currentLine_));
		}
		else if (*currentPos_ == ':') {
			currentPos_++;
			std::cout << ": found" << std::endl;
			return (cToken(COLON, ":", currentLine_));
		}
		else if (*currentPos_ == ';') {
			currentPos_++;
			std::cout << "; found" << std::endl;
			return (cToken(SEMICOLON, ";", currentLine_));
		}
		else if (*currentPos_ == '/') {
			currentPos_++;
			std::cout << "/ found" << std::endl;
			return (cToken(SLASH, "/", currentLine_));
		}
		else if (isdigit(*currentPos_)) {
			while (currentPos_ != allConfig_.end() && *currentPos_ != '\n' && (isdigit(*currentPos_) || *currentPos_ == '.')) {
				tValue += *currentPos_;
				currentPos_++;			}
			std::cout << tValue << " found" << std::endl;
			return (cToken(NUMBER, tValue, currentLine_));
		}
		else if (isalpha(*currentPos_)) {
			while (currentPos_ != allConfig_.end() && *currentPos_ != '\n' && (isalpha(*currentPos_) || isdigit(*currentPos_) || *currentPos_ == '-' || *currentPos_ == '_' || *currentPos_ == '.')) {
				tValue.push_back(*currentPos_);
				currentPos_++;
			}
			std::cout << tValue << " found" << std::endl;
			return (cToken(STRING, tValue, currentLine_));
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
			// std::cout << "Unknown input: " << tValue << " found on line " << currentLine_ << std::endl;
			return (cToken(UNKNOWN, tValue, currentLine_));
		}
	}
	if (currentPos_ == allConfig_.end()) {
		std::cout << "EOF found" << std::endl;
		return (cToken(END_OF_FILE, "", currentLine_));
	}
	return (cToken(UNKNOWN, "", currentLine_)); //it shouldn't reach here, but it's not a void funciton, so it needs to return sth
}

/// @brief checks if the token list is empty, checks if th number of pairs of brackets if correct
/// @return 
int	ConfParser::tokensCheck() {
	
	if (allTokens_.empty()) {
		std::cerr << "Error: Incorrect config file: no tokens retrived" << std::endl;
		return (EXIT_FAILURE);
	}
	//checking the the braces are matched
	int	braceCount(0);
	int	openBraceLine(0);
	for (size_t i(0); i < allTokens_.size(); i++) {
		if (allTokens_[i].type == OPEN_BRACE) {
			braceCount++;
			openBraceLine = allTokens_[i].line;
		}
		if (allTokens_[i].type == CLOSE_BRACE) {
			braceCount--;
			if (braceCount < 0) {
				std::cerr << "Error: Incorrect config file: unmatched closing brace '}' at line " << allTokens_[i].line << std::endl;
				return (EXIT_FAILURE);
			}
		}
	}
	if (braceCount > 0) {
		std::cerr << "Error: Incorrect config file: unmatched opening brace '{' at line " << openBraceLine << std::endl;
		return (EXIT_FAILURE);
	}

	return (EXIT_SUCCESS);
}

int	ConfParser::setAllTokens() {
	
	cToken	currentToken;
	while (currentPos_ < allConfig_.end()) {
		currentToken = defineToken();
		if (currentToken.type == UNKNOWN) {
			std::cerr << "Error: Incorrect config file: Unknown token '" << currentToken.value << "' at line " << currentToken.line << std::endl;
			return (EXIT_FAILURE);
		}
		allTokens_.push_back(currentToken);
	}
	if (tokensCheck() == EXIT_FAILURE) {
		return (EXIT_FAILURE);
	}
	for (size_t i = 0; i < allTokens_.size(); i++) {
		std::cout << "Token: " << allTokens_[i].type << "\tValue: " << allTokens_[i].value << std::endl;
	}
	return (EXIT_SUCCESS);
}

// int	ConfParser::populateLocation(SingleServer& server, size_t& i) {
	
	
// 	// add the location's values to the server
// 	return (EXIT_SUCCESS);
// }

/// @brief 
/// @param servers 
/// @param i starts with the next token after "server"
/// @return 
int	ConfParser::populateServers(Server42& servers, size_t& i) {
	
	i++; //moving to the open brace token
	if (allTokens_[i].type != OPEN_BRACE) {
		std::cerr << "Error: Incorrect config file: expected '{' after 'server' directive at line: " << allTokens_[i].line << std::endl;
		return (EXIT_FAILURE);
	}
	i++; // go to the next token after the opening brace
	SingleServer	newServer;

	while (i < allTokens_.size() && allTokens_[i].value != "server") {
		if (allTokens_[i].type == STRING) {
			if (allTokens_[i].value == "listen") {
				i++; //move to the token after listen
				if (allTokens_[i].type != NUMBER || allTokens_[i].value != "localhost") {
					std::cerr << "Error: Incorrect config file: expected IP/localhost and port number after 'listen' directive at line " << allTokens_[i].line << std::endl;
					return (EXIT_FAILURE);
				}
				else if (allTokens_[i].value == "localhost") {
					newServer.setServIP("127.0.0.1");
				}
				else {
					newServer.setServIP(allTokens_[i].value);
				}
				i++; // gets to the colon or the next token
				if (allTokens_[i].type == COLON) {
					i++; // move to the port number token
					if (allTokens_[i].type != NUMBER) {
						std::cerr << "Error: Incorrect config file: expected port number after ':' at line " << allTokens_[i].line << std::endl;
						return (EXIT_FAILURE);
					}
					else {
						newServer.setServPortString(allTokens_[i].value);
						newServer.setServPortInt(std::stoi(allTokens_[i].value));
						i++;
					}
				}
				//somewhere here may be still semicolon - need to check later, now need to go
				else
					continue; //should get back to the while loop
			}
			else if (allTokens_[i].value == "server_name") {
				//populate the server's name
			}
			else if (allTokens_[i].value == "location") {
				// if (populateLocation(newServer, i) == EXIT_FAILURE) {
				// 	return (EXIT_FAILURE);
				// }
				//populate the server's location
			}
			else if (allTokens_[i].value == "client_max_body_size") {
				//populate the server's max body size
			}
			else if (allTokens_[i].value == "error_page") {
				//populate the server's error pages
			}
			else {
				std::cerr << "Error: Incorrect config file: unknown directive ;" << allTokens_[i].value << "' at line " << allTokens_[i].line << std::endl;
				return (EXIT_FAILURE);
			}
		}
		else if (allTokens_[i].type == END_OF_FILE) {
			return (EXIT_SUCCESS);
		}
		i++;
	}
	servers.addServer(newServer);
	
	return (EXIT_SUCCESS);
}

int ConfParser::parseConfig(Server42& servers) {
	
	if (setAllTokens() == EXIT_FAILURE) {
		std::cerr << "Error following up - from setAllTokens() in parseConfig()" << std::endl;
		return (EXIT_FAILURE);
	}
	for (size_t i(0); allTokens_[i].type != END_OF_FILE; i++) {
		if (allTokens_[i].type == STRING && allTokens_[i].value == "server") {
			if (populateServers(servers, i) == EXIT_FAILURE) {
				std::cerr << "Error following up - from populateServers() in parseConfig()" << std::endl;
				return (EXIT_FAILURE);
			}
		}
		//TODO - will add here further populating things in the further stages of the project 
		// e.g for the files we have to pass, etc
		else {
			std::cerr << "Error: Incorrect config file: unexpected token at line " << allTokens_[i].line << std::endl;
			return (EXIT_FAILURE);
		}
	}
	return (EXIT_SUCCESS);
}
