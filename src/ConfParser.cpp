/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ConfParser.cpp                                     :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/06/12 11:27:51 by mstencel      #+#    #+#                 */
/*   Updated: 2025/07/07 15:12:49 by mstencel      ########   odam.nl         */
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

//exception class
ConfParser::ConfParserException::ConfParserException(const std::string& message, const std::string& token, size_t line) {
	allMessage_ = "Error: Incorrect config file: " + message + token + " at line " + std::to_string(line);
}

ConfParser::ConfParserException::ConfParserException(const std::string& message, size_t line) {
	allMessage_ = "Error: Incorrect config file: " + message + " at line " + std::to_string(line);
}

ConfParser::ConfParserException::ConfParserException(const std::string& message) {
	allMessage_ = "Error: Incorrect config file: " + message;
}

const char* ConfParser::ConfParserException::what() const throw() {
	return (allMessage_.c_str());
}

ConfParser::ConfParserException::~ConfParserException() {
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
			if (*currentPos_ == '\n')
				currentLine_++;
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
		else if (isalpha(*currentPos_) || *currentPos_ == '/') {
			while (currentPos_ != allConfig_.end() && *currentPos_ != '\n' && *currentPos_ != ' ' && *currentPos_ != ';' && isprint(*currentPos_) ) {
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
			// std::cout << "Unknown input: " << tValue << " found on line " << currentLine_ << std::endl;
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
void	ConfParser::validateBraces() {
	
	if (allTokens_.empty()) {
		throw ConfParser::ConfParserException("no tokens retrived ", 0);
		// std::cerr << "Error: Incorrect config file: no tokens retrived" << std::endl;
		// return (EXIT_FAILURE);
	}
	//checking the the braces are matched
	int	braceCount(0);
	int	openBraceLine(0);
	
	for (const auto& token : allTokens_) {
		if (token.type == OPEN_BRACE) {
			braceCount++;
			openBraceLine = token.line;
		}
		else if (token.type == CLOSE_BRACE) {
			braceCount--;
			if (braceCount < 0) {
				throw ConfParser::ConfParserException("unmatched closing brace ", token.line);
				// std::cerr << "Error: Incorrect config file: unmatched closing brace '}' at line " << token.line << std::endl;
				// return (EXIT_FAILURE);
			}
		}
	}
	if (braceCount > 0) {
		throw ConfParser::ConfParserException("unmatched opening brace ", openBraceLine);
		// std::cerr << "Error: Incorrect config file: unmatched opening brace '{' at line " << openBraceLine << std::endl;
		// return (EXIT_FAILURE);
	}

	// return (EXIT_SUCCESS);
}

void	ConfParser::setAllTokens() {
	
	cToken	currentToken;
	while (currentPos_ < allConfig_.end()) {
		currentToken = defineToken();
		if (currentToken.type == UNKNOWN) {
			throw ConfParser::ConfParserException("Unknown token: ", currentToken.value, currentToken.line);
		}
		if (currentToken.type == STRING) {
			int allTokenSize = (int)allTokens_.size(); 
			if (allTokenSize == 0 ||
				allTokens_[allTokenSize - 1].type == SEMICOLON ||
				allTokens_[allTokenSize - 1].type == CLOSE_BRACE ||
				allTokens_[allTokenSize - 1].type == OPEN_BRACE) {
				currentToken.type = DIRECTIVE;
			}
		}
		allTokens_.push_back(currentToken);
	}
	try {
		validateBraces();
	}
	catch (const ConfParser::ConfParserException& e) {
			std::cerr << e.what() << std::endl;
	}
	// if (validateBraces() == EXIT_FAILURE) {
	// 	return (EXIT_FAILURE);
	// }


	// to remove - printing the tokens
	// for (const auto& token : allTokens_) {
	// 	std::cout << "Token: " << token.type << "\tValue: " << token.value << std::endl;
	// }
	
	// return (EXIT_SUCCESS);
}

int	ConfParser::semicolonCheck(const tokenType& type, size_t line) {
	if (type != SEMICOLON) {
		throw ConfParser::ConfParserException("expected ';' after the value ", line);
		// std::cerr << "Error: Incorrect config file: expected ';' after the value at the end of line " << line << std::endl;
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}

int	ConfParser::validateIP(const std::string& ip, size_t line) {

	int	dotCount(0);
	for (const auto& c : ip) {
		if (c == '.') {
			dotCount++;
		}
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
	// std::cout << "ip set: " << newServer.getServIP() << std::endl;
	// std::cout << "ip token value " << allTokens_[i].value << std::endl;
	i++; // moved to the semicolon?
	// std::cout << "semicolong token value " << allTokens_[i].value << std::endl;
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

/// @brief checks and sets the port using only the 1st one in the file
/// @param newServer 
/// @param i 
/// @return 
int	ConfParser::addPort(SingleServer& newServer, size_t& i) {
	
	i++; //moved to the token after "port"
	// std::cout << "current token n " << i + 1 << ": " << allTokens_[i].value << std::endl;
	// std::cout << "current token type " << allTokens_[i].type << std::endl;
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
	// std::cout << "port set: " << newServer.getServPortInt() << std::endl;
	// std::cout << "port token value " << allTokens_[i].value << std::endl;
	i++; //moved to the semicolon?
	// std::cout << "semicolon token value " << allTokens_[i].value << std::endl << std::endl;
	return (semicolonCheck(allTokens_[i].type, allTokens_[i].line));
}

int	ConfParser::addServerName(SingleServer& newServer, size_t& i) {
	i++; //moving to the token after 'server_name'
	// std::cout << "in addServerName() ";
	// std::cout << "current token n " << i + 1 << ": " << allTokens_[i].value << std::endl;
	// std::cout << "current token type " << allTokens_[i].type << std::endl;
	if (allTokens_[i].type != STRING) {
		std::cerr << "Error: Incorrect config file: expected string after server_name directive at line " << allTokens_[i].line << std::endl;
		return (EXIT_FAILURE);
	}
	newServer.setServName(allTokens_[i].value);
	// std::cout << "server set: " << newServer.getServName() << std::endl;
	// std::cout << "server token value " << allTokens_[i].value << std::endl;
	i++; //moved to the semicolon?
	// std::cout << "semicolon token value " << allTokens_[i].value << std::endl << std::endl;
	return (semicolonCheck(allTokens_[i].type, allTokens_[i].line));
}

int	ConfParser::addLocation(SingleServer& server, size_t& i) {
	i++; // move to the location's path
	// std::cout << "location's path token's value " << allTokens_[i].value << std::endl;
	if (allTokens_[i].type != STRING) {
		std::cerr << "Error: Incorrect config file: expected location's path after 'location' directive at line " << allTokens_[i].line << std::endl;
		return (EXIT_FAILURE);
	}
	Location newLocation;
	newLocation.setPath(allTokens_[i].value);
	// std::cout << "path set: " << newLocation.getPath() << std::endl;
	// std::cout << "path token value " << allTokens_[i].value << std::endl;
	i++; // move to the opening brace
	// std::cout << "open brace token value " << allTokens_[i].value << std::endl << std::endl;
	if (allTokens_[i].type != OPEN_BRACE) {
		std::cerr << "Error: Incorrect config file: expected '{' after location's path at line " << allTokens_[i].line << std::endl;
		return (EXIT_FAILURE);
	}
	i++; //move to the next token
	while (allTokens_[i].type != CLOSE_BRACE) {
		// std::cout << "directive token value " << allTokens_[i].value << std::endl;
		if (allTokens_[i].value == "root") {
			i++; // move to the next value
			if (allTokens_[i].type != STRING) {
				std::cerr << "Error: Incorrect config file: expected a string value after root directive at line " << allTokens_[i].line << std::endl;
				return (EXIT_FAILURE);
			}
			newLocation.setRoot(allTokens_[i].value);
			// std::cout << "root set: " << newLocation.getRoot() << std::endl;
			// std::cout << "root token value " << allTokens_[i].value << std::endl;
			i++; //move to the semicolon?
			// std::cout << "semicolon token value " << allTokens_[i].value << std::endl << std::endl;
			if (semicolonCheck(allTokens_[i].type, allTokens_[i].line) == EXIT_FAILURE)
				return (EXIT_FAILURE);
		}
		else if (allTokens_[i].value == "index") {
			i++; //move to the index value
			if (allTokens_[i].type != STRING) {
				std::cerr << "Error: Incorrect config file: expected a string value after index directive at line " << allTokens_[i].line << std::endl;
				return (EXIT_FAILURE);
			}
			newLocation.setIndex(allTokens_[i].value);
			// std::cout << "index set: " << newLocation.getPath() << std::endl;
			// std::cout << "index token value " << allTokens_[i].value << std::endl;
			i++; //move to the semicolon?
			// std::cout << "semicolon token value " << allTokens_[i].value << std::endl << std::endl;
			if (semicolonCheck(allTokens_[i].type, allTokens_[i].line) == EXIT_FAILURE)
				return (EXIT_FAILURE);
		}
		else if (allTokens_[i].value == "allowed_methods") {
			i++;
			if (allTokens_[i].type != STRING) {
				std::cerr << "Error: Incorrect config file: expected a string value after index directive at line " << allTokens_[i].line << std::endl;
				return (EXIT_FAILURE);
			}
			while (allTokens_[i].type == STRING) {
				if (allTokens_[i].value != "GET" && allTokens_[i].value != "POST" && allTokens_[i].value != "DELETE") {
					// std::cout << "index token value " << allTokens_[i].value << std::endl;
					std::cerr << "Error: Incorrect config file: expected one of valid allowed_methods (GET, POST or DELETE) at line " << allTokens_[i].line << std::endl;
					return (EXIT_FAILURE);
				}
				newLocation.setAllowedMethods(allTokens_[i].value);
				// std::cout << "index token value " << allTokens_[i].value << std::endl;
				i++; //move to the semicolon?
				// std::cout << "semicolon token value " << allTokens_[i].value << std::endl << std::endl;
			}
			if (semicolonCheck(allTokens_[i].type, allTokens_[i].line) == EXIT_FAILURE)
				return (EXIT_FAILURE);
		}
		else if (allTokens_[i].value == "autoindex") {
			i++;
			if (allTokens_[i].type != STRING) {
				std::cerr << "Error: Incorrect config file: expected a string value after index directive at line " << allTokens_[i].line << std::endl;
				return (EXIT_FAILURE);
			}
			if (allTokens_[i].value == "on") {
				newLocation.setAutoindex(true);
			}
			else if (allTokens_[i].value == "off") {
				newLocation.setAutoindex(false);
			}
			else {
				std::cerr << "Error: Incorrect config file: expected 'on' or 'off' after autoindex directive at line " << allTokens_[i].line << std::endl;
				return (EXIT_FAILURE);
			}
			// std::cout << "autoindex set: " << newLocation.getAutoindex() << std::endl;
			// std::cout << "index token value " << allTokens_[i].value << std::endl;
			i++; //move to the semicolon?
			// std::cout << "semicolon token value " << allTokens_[i].value << std::endl << std::endl;
			if (semicolonCheck(allTokens_[i].type, allTokens_[i].line) == EXIT_FAILURE)
				return (EXIT_FAILURE);
		}
		else if (allTokens_[i].value == "return") {
			i++; //move to the redirect code
			if (allTokens_[i].type != NUMBER || (allTokens_[i].value != "301" && allTokens_[i].value != "302")) {
				std::cerr << "Error: Incorrect config file: expected the redirection code 301 or 302 after 'return' at line " << allTokens_[i].line << std::endl;
				return (EXIT_FAILURE);
			}
			newLocation.setRedirectionCode(std::stoi(allTokens_[i].value));
			i++; //move to the redirect path
			if (allTokens_[i].type != STRING) {
				std::cerr << "Error: Incorrect config file: exptected the redirection path after the redirection code at line " << allTokens_[i].line << std::endl;
				return (EXIT_FAILURE);
			}
			newLocation.setRedirectionsPath(allTokens_[i].value);
			i++; //move to the semicolon
			if (semicolonCheck(allTokens_[i].type, allTokens_[i].line) == EXIT_FAILURE) {
				return (EXIT_FAILURE);
			}
		}
		else if (allTokens_[i].value == "upload_path") {
			i++; //move to the upload path
			// std::cout << "upload path = " << allTokens_[i].value << std::endl;
			if (allTokens_[i].type != STRING) {
				std::cerr << "Error: Incorrect config file: expected a string after 'upload_path directive at line " << allTokens_[i].line << std::endl;
				return (EXIT_FAILURE);
			}
			newLocation.setUploadPath(allTokens_[i].value);
			// std::cout << "upload_path set: " << newLocation.getUploadPath() << std::endl;
			i++; //move to the semicolon
			if (semicolonCheck(allTokens_[i].type, allTokens_[i].line) == EXIT_FAILURE) {
				return (EXIT_FAILURE);
			}
		}
		i++; //move after the semicolon
		//TODO add cgi-related stuff
	}
	if (allTokens_[i].type != CLOSE_BRACE) {
		std::cerr << "Error: Incorrect config file: expected '}' at the end of the location block at line " << allTokens_[i].line << std::endl;
		return (EXIT_FAILURE);
	}
	// std::cout << "did I get out from the location?" << std::endl;
	std::cout << newLocation << std::endl;
	server.setLocations(newLocation);
	return (EXIT_SUCCESS);
}

int	ConfParser::addMaxBodySize(SingleServer& newServer, size_t& i) {
	i++; //move to the next token
	// std::cout << "max body size value: " << allTokens_[i].value << std::endl;
	if (allTokens_[i].type != NUMBER) {
		std::cerr << "Error: Incorrect config file: expected a number after 'client_max_body_size' directive at line " << allTokens_[i].line << std::endl;
		return (EXIT_FAILURE);
	}
	if (allTokens_[i].value.length() > 8) {
		std::cerr << "Error: Incorrect config file: 'client_max_body_size' value is too big at line " << allTokens_[i].line << std::endl;
		return (EXIT_FAILURE);
	}
	int	maxSize = std::stoi(allTokens_[i].value);
	if (maxSize < 0 || maxSize > 10485760) {
		std::cerr << "Error: Incorrect config file: 'client_max_body_size' value can be between 1 - 10485760 at line " << allTokens_[i].line << std::endl;
		return (EXIT_FAILURE);
	}
	newServer.setMaxBodySize(maxSize);
	// std::cout << "max body size set: " << newServer.getMaxBodySize() << std::endl;
	// std::cout << "max body's token value " << allTokens_[i].value << std::endl;
	i++; //moved to the semicolon?
	// std::cout << "semicolon token value " << allTokens_[i].value << std::endl << std::endl;
	return (semicolonCheck(allTokens_[i].type, allTokens_[i].line));
}

int	ConfParser::addErrorPages(SingleServer& newServer, size_t& i) {
	i++; // move to the next token
	if (allTokens_[i].type != NUMBER && allTokens_[i].value.length() != 3) {
		//TODO change it according to our error pages
		std::cerr << "Error: Incorrect config file: expected error page code value after error_page directive at line " << allTokens_[i].line << std::endl;
		return (EXIT_FAILURE);
	}
	if (allTokens_[i + 1].type != STRING) {
		std::cerr << "Error: Incorrect config file: expected string after error_page code directive at line " << allTokens_[i].line << std::endl;
		return (EXIT_FAILURE);
	}
	int	errorCode = std::stoi(allTokens_[i].value);
	i++; // move to the error page path
	newServer.setErrorPages(errorCode, allTokens_[i].value);;
	// std::cout << "error page code set to: " << errorCode << std::endl;
	// std::cout << "error page address set to: " << allTokens_[i].value << std::endl;
	i++; //moved to the semicolon?
	// std::cout << "semicolon token value " << allTokens_[i].value << std::endl << std::endl;
	return (semicolonCheck(allTokens_[i].type, allTokens_[i].line));
}


/// @brief in the server block, goes from '{' until '}' and populates the single server and adds it in vector with servers
/// @param servers vector containing all servers
/// @param i current token's position
/// @return failure or success
int	ConfParser::populateServers(Server42& servers, size_t& i) {
	
	i++; //moving to the open brace token
	if (allTokens_[i].type != OPEN_BRACE) {
		std::cerr << "Error: Incorrect config file: expected '{' after 'server' directive at line: " << allTokens_[i].line << std::endl;
		return (EXIT_FAILURE);
	}
	i++; // go to the next token after the opening brace
	// std::cout << "starting with? current token n " << i + 1 << ": " << allTokens_[i].value << std::endl;
	SingleServer	newServer;
	
	// std::cout << "hello populateServers()" << std::endl;

	while (i < allTokens_.size() && allTokens_[i].value != "server") {
		if (allTokens_[i].type == DIRECTIVE) {
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
					// std::cout << "did I get further?" << std::endl;
					continue;
				}
			}
			else if (allTokens_[i].value == "server_name") {
				// std::cout << "getting the server's name?" << std::endl;
				if (addServerName(newServer, i) == EXIT_FAILURE) {
					// std::cout << "i really shouldn't be here" << std::endl;
					return (EXIT_FAILURE);
				}
				else {
					// std::cout << "I'm in the good spot" << std::endl;
					i++; //move to the token after the semicolon
					continue;
				}
			}
			else if (allTokens_[i].value == "location") {
				// std::cout << "in the location" << std::endl;
				// std::cout << "it shouldn't be ; " << allTokens_[i].value << std::endl;
				if (addLocation(newServer, i) == EXIT_FAILURE) {
					return (EXIT_FAILURE);
				}
				else {
					i++; //move to the token after the seimicolon
					// std::cout << "after location now is: " << allTokens_[i].value << std::endl;
					continue;
				}
			}
			else if (allTokens_[i].value == "client_max_body_size") {
				// std::cout << "in the body size" << std::endl;
				if (addMaxBodySize(newServer, i) == EXIT_FAILURE) {
					return (EXIT_FAILURE);
				}
				else {
					i++; //move to the token after the seimicolon
					// std::cout << "after max_body_size now is: " << allTokens_[i].value << std::endl;
					continue;
				}
			}
			else if (allTokens_[i].value == "error_page") {
				if (addErrorPages(newServer, i) == EXIT_FAILURE) {
					return (EXIT_FAILURE);
				}
				else {
					i++; //move to the token after the seimicolon
					// std::cout << "after error_page now is: " << allTokens_[i].value << std::endl;
					continue;
				}
			}
			else {
				std::cerr << "Error: Incorrect config file: unknown directive ;" << allTokens_[i].value << "' at line " << allTokens_[i].line << std::endl;
				return (EXIT_FAILURE);
			}
		}
		else if (allTokens_[i].type == CLOSE_BRACE) {
			// std::cout << " did I get here? closing brace of the server?" << std::endl;
			i++; //move to the next token after the closing brace
			servers.addServer(newServer);
			return (EXIT_SUCCESS);
		}
		else {
			break;
		}
		i++;
	}
	std::cerr << "Error: Incorrect config file: unexpected token '" << allTokens_[i].value << "' at line " << allTokens_[i].line << std::endl;
	return (EXIT_FAILURE);
}

int ConfParser::parseConfig(Server42& servers) {
	
	// std::cout << "hello in parseConfig()" << std::endl;
	try {
		setAllTokens();
	}
	catch (const ConfParserException& e) {
		std::cerr << e.what() << std::endl;
		return (EXIT_FAILURE);
	}
	// if (setAllTokens() == EXIT_FAILURE) {
	// 	std::cerr << "Error following up - from setAllTokens() in parseConfig()" << std::endl;
	// 	return (EXIT_FAILURE);
	// }
	
	size_t i(0);
	while (allTokens_[i].type != END_OF_FILE)

	{
		// std::cout << "token in parseConfig(): " << allTokens_[i].value << std::endl;
		if (allTokens_[i].type == DIRECTIVE && allTokens_[i].value == "server") {
			if (populateServers(servers, i) == EXIT_FAILURE) {
				std::cerr << "Error following up - from populateServers() in parseConfig()" << std::endl;
				return (EXIT_FAILURE);
			}
			else {
				// std::cout << "I should get here cause server is done" << std::endl;
				continue;
			}
		}
		else {
			std::cerr << "Error: Incorrect config file: unexpected token at line " << allTokens_[i].line << std::endl;
			return (EXIT_FAILURE);
		}
		i++;
	}
	return (EXIT_SUCCESS);
}
