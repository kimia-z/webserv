/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ConfParser.cpp                                     :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/06/12 11:27:51 by mstencel      #+#    #+#                 */
/*   Updated: 2025/06/27 14:45:42 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../incl/ConfParser.hpp"

ConfParser::ConfParser(): currentLine_(1) {
	// std::cout << "ConfParser default constructor called" << std::endl;
}

ConfParser::ConfParser(const std::string& config):
	allConfig_(config),
	currentPos_(allConfig_.begin()),
	currentLine_(1) {
		// std::cout << "ConfParser constructor with param called" << std::endl;
		// std::cout << allConfig_ << std::endl;
		// std::cout << "Current position: " << *currentPos_ << std::endl;
}

ConfParser::ConfParser(const ConfParser& copy):
	allConfig_(copy.allConfig_),
	currentPos_(copy.currentPos_),
	currentLine_(copy.currentLine_),
	allTokens_(copy.allTokens_) {
		// std::cout << "ConfParser copy constructor called" << std::endl;
}

ConfParser& ConfParser::operator=(const ConfParser& copy) {
	if (this != &copy) {
		allConfig_ = copy.allConfig_;
		currentPos_ = copy.currentPos_;
		currentLine_ = copy.currentLine_;
		allTokens_ = copy.allTokens_;
	}
	// std::cout << "ConfParser copy assignment operator called" << std::endl;
	return (*this);
}

ConfParser::~ConfParser() {
	// std::cout << "ConfParser default destructor called" << std::endl;
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
		if (*currentPos_ == ' ' || *currentPos_ == '\t')
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
			// std::cout << "EOF found" << std::endl;
			tValue = "";
			return (cToken(END_OF_FILE, tValue, currentLine_));
		}

		char	currentChar(*currentPos_);
		switch (currentChar) {
			case '{':
				currentPos_++;
				// std::cout << "{ found" << std::endl;
				tValue = "{";
				return (cToken(OPEN_BRACE, tValue, currentLine_));
			case '}':
				currentPos_++;
				// std::cout << "} found" << std::endl;
				tValue = "}";
				return (cToken(CLOSE_BRACE, tValue, currentLine_));
			case ':':
				currentPos_++;
				// std::cout << ": found" << std::endl;
				tValue = ":";
				return (cToken(COLON, tValue, currentLine_));
			case ';':
				currentPos_++;
				// std::cout << "; found" << std::endl;
				tValue = ";";
				return (cToken(SEMICOLON, tValue, currentLine_));
			case '/':
				currentPos_++;
				// std::cout << "/ found" << std::endl;
				tValue = "/";
				return (cToken(SLASH, tValue, currentLine_));
			case '\n':
				currentLine_++;
				currentPos_++;
				// std::cout << "\\n found" << std::endl;
				continue; // skip to the next character
			default:
				break;
		}
		if (isdigit(*currentPos_)) {
			while (currentPos_ != allConfig_.end() && *currentPos_ != '\n' && (isdigit(*currentPos_) || *currentPos_ == '.')) {
				tValue += *currentPos_;
				currentPos_++;			}
			// std::cout << tValue << " found" << std::endl;
			return (cToken(NUMBER, tValue, currentLine_));
		}
		else if (isalpha(*currentPos_)) {
			while (currentPos_ != allConfig_.end() && *currentPos_ != '\n' && (isalpha(*currentPos_) || isdigit(*currentPos_) || *currentPos_ == '-' || *currentPos_ == '_' || *currentPos_ == '.')) {
				tValue.push_back(*currentPos_);
				currentPos_++;
			}
			// std::cout << tValue << " found" << std::endl;
			return (cToken(STRING, tValue, currentLine_));
		}
		else {
			while (currentPos_ != allConfig_.end() && *currentPos_ != '\n') {
				tValue += *currentPos_;
				currentPos_++;
			}
			std::cout << "Unknown input: " << tValue << " found on line " << currentLine_ << std::endl;
			return (cToken(UNKNOWN, tValue, currentLine_));
		}
	}
		if (currentPos_ == allConfig_.end()) {
		// std::cout << "EOF found" << std::endl;
		tValue = "";
		return (cToken(END_OF_FILE, tValue, currentLine_));
	}
	tValue = "";
	return (cToken(UNKNOWN, tValue, currentLine_)); //it shouldn't reach here, but it's not a void funciton, so it needs to return sth
}

/// @brief checks if the token list is empty, checks if the number of brackets' pairs is correct
/// @return EXIT_FAILURE upon the error, EXIT_SUCCESS if all is good
int	ConfParser::validateTokens() {
	
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
	// to remove - printing the tokens
	// for (size_t i = 0; i < allTokens_.size(); i++) {
	// 	std::cout << "Token: " << allTokens_[i].type << "\tValue: " << allTokens_[i].value << std::endl;
	// }
	return (EXIT_SUCCESS);
}

int	ConfParser::semicolonCheck(const tokenType& type, size_t line) {
	if (type != SEMICOLON) {
		std::cerr << "Error: Incorrect config file: expected ';' after the value at the end of line " << line << std::endl;
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}

int	ConfParser::validateIP(const std::string& ip, size_t line) {
	
	//count the dots
	//check if the numbers are within 0-255
	
	int	dotCount(0);
	for(size_t i(0); i < ip.size(); i++) {
		if (ip[i] == '.')
		dotCount++;
	}
	if (dotCount != 3) {
		std::cerr << "Error: Incorrect config file: invalid IP address: " << ip << " at line " << line << std::endl;
		return (EXIT_FAILURE);
	}
	
	std::string	ipPart;
	int	start(0);
	int	end(0);
	int	ipPartInt(0);
	for (int i(0); i < 4; i++) {
		if (i < 3) {
			end = ip.find('.', start);
			ipPart = ip.substr(start, end - start);
		}
		else {
			ipPart = ip.substr(start);
		}
		if (ipPart.empty() || ipPart.find_first_not_of("0123456789") != std::string::npos) {
			std::cerr << "Error: Incorrect config file: invalid IP address: " << ip << " at line " << line << std::endl;
			return (EXIT_FAILURE);
		}
		ipPartInt = std::stoi(ipPart);
		// std::cout << ipPartInt << std::endl;
		if (ipPartInt < 0 || ipPartInt > 255) {
			std::cerr << "Error: Incorrect config file: invalid IP address: " << ip << " at line " << line << std::endl;
			return (EXIT_FAILURE);
		}
		start = end + 1;
	}
	return (EXIT_SUCCESS);
}

int	ConfParser::addIP(SingleServer& newServer, size_t& i) {
	i++; //move to the token after listen
	if (allTokens_[i].type != NUMBER && allTokens_[i].value != "localhost") {
		std::cerr << "Error: Incorrect config file: expected IP/localhost and port number after 'listen' directive at line " << allTokens_[i].line << std::endl;
		return (EXIT_FAILURE);
	}
	else if (allTokens_[i].value == "localhost") {
		newServer.setServIP("127.0.0.1");
	}
	else {
		if (validateIP(allTokens_[i].value, allTokens_[i].line) == EXIT_FAILURE) {
			return (EXIT_FAILURE);
		}
		newServer.setServIP(allTokens_[i].value);
	}
	i++; // moved to the semicolon?
	return (semicolonCheck(allTokens_[i].type, allTokens_[i].line));
}

int	ConfParser::validatePort(const std::string& port, size_t line) {
	
	int	portInt(std::stoi(port));
	if (portInt < 1024 || portInt > 65535) {
		std::cerr << "Error: Incorrect config file: invalid port number: " << port << " at line " << line << std::endl;
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}

int	ConfParser::addPort(SingleServer& newServer, size_t& i) {
	
	i++; //moved to the token after "port"
	std::cout << "current token n " << i + 1 << ": " << allTokens_[i].value << std::endl;
	if (allTokens_[i].type == STRING && allTokens_[i].value == "port") {
		i++; // move to the port number token
		std::cout << "current token n " << i + 1 << ": " << allTokens_[i].value << std::endl;
		if (allTokens_[i].type != NUMBER) {
			std::cerr << "Error: Incorrect config file: expected port number after 'port' at line " << allTokens_[i].line << std::endl;
			return (EXIT_FAILURE);
		}
		else {
			if (validatePort(allTokens_[i].value, allTokens_[i].line) == EXIT_FAILURE) {
				return (EXIT_FAILURE);
			}
			if (newServer.getServPortInt() == -1) {
				newServer.setServPortString(allTokens_[i].value);
				newServer.setServPortInt(std::stoi(allTokens_[i].value));
			}
		}
		}
	i++; //moved to the semicolon?
	return (semicolonCheck(allTokens_[i].type, allTokens_[i].line));
}

int	ConfParser::addServerName(SingleServer& newServer, size_t& i) {
	i++; //moving to the token after 'server_name'
	newServer.setServName(allTokens_[i].value);
	i++; //moving to the semicolon?
	return (semicolonCheck(allTokens_[i].type, allTokens_[i].line));
}

int	ConfParser::populateLocation(SingleServer& server, size_t& i) {
	i++; // move to the location's path
	
	
	
	// add the location's values to the server
	return (EXIT_SUCCESS);
}

int	ConfParser::populateServers(Server42& servers, size_t& i) {
	
	i++; //moving to the open brace token
	if (allTokens_[i].type != OPEN_BRACE) {
		std::cerr << "Error: Incorrect config file: expected '{' after 'server' directive at line: " << allTokens_[i].line << std::endl;
		return (EXIT_FAILURE);
	}
	i++; // go to the next token after the opening brace
	std::cout << "starting with? current token n " << i + 1 << ": " << allTokens_[i].value << std::endl;
	SingleServer	newServer;
	
	while (i < allTokens_.size() && allTokens_[i].value != "server") {
		if (allTokens_[i].type == STRING) {
			//all the adding functions will end with the semicolon token
			if (allTokens_[i].value == "listen") {
				if (addIP(newServer, i) == EXIT_FAILURE) {
					return (EXIT_FAILURE);
				}
				else {
					i++; //move to the token after the semicolon
					continue;
				}
			}
			else if (allTokens_[i].value == "port") {
				if (addPort(newServer, i) == EXIT_FAILURE) {
					return (EXIT_FAILURE);
				}
				else {
					i++; //move to the token after the semicolon
					continue;
				}
			}
			else if (allTokens_[i].value == "server_name") {
				if (addServerName(newServer, i) == EXIT_FAILURE) {
					return (EXIT_FAILURE);
				}
				else {
					i++; //move to the token after the semicolon
					continue;
				}
			}
			else if (allTokens_[i].value == "location") {
				// std::cout << "in the location" << std::endl;
				if (populateLocation(newServer, i) == EXIT_FAILURE) {
					return (EXIT_FAILURE);
				}
				else {
					i++; //move to the token after the seimicolon
					continue;
				}
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
		else if (semicolonCheck(allTokens_[i].type, allTokens_[i].line)) {
			std::cerr << "Error: Incorrect config file: extra ';' at line " << allTokens_[i].line << std::endl;
			return (EXIT_FAILURE);
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
