/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ConfParser.cpp                                     :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/06/12 11:27:51 by mstencel      #+#    #+#                 */
/*   Updated: 2025/07/11 09:58:43 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../incl/ConfParser.hpp"

ConfParser::ConfParser(): currentLine_(1) {
}

ConfParser::ConfParser(const std::string& config):
	allConfig_(config),
	currentPos_(allConfig_.begin()),
	currentLine_(1) {
}

ConfParser::ConfParser(const ConfParser& copy):
	allConfig_(copy.allConfig_),
	currentPos_(copy.currentPos_),
	currentLine_(copy.currentLine_),
	allTokens_(copy.allTokens_) {
}

ConfParser& ConfParser::operator=(const ConfParser& copy) {
	if (this != &copy) {
		allConfig_ = copy.allConfig_;
		currentPos_ = copy.currentPos_;
		currentLine_ = copy.currentLine_;
		allTokens_ = copy.allTokens_;
	}
	return (*this);
}

ConfParser::~ConfParser() {
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
			tValue = "";
			return (cToken(END_OF_FILE, tValue, currentLine_));
		}

		char	currentChar(*currentPos_);
		switch (currentChar) {
			case '{':
				currentPos_++;
				tValue = "{";
				return (cToken(OPEN_BRACE, tValue, currentLine_));
			case '}':
				currentPos_++;
				tValue = "}";
				return (cToken(CLOSE_BRACE, tValue, currentLine_));
			case ':':
				currentPos_++;
				tValue = ":";
				return (cToken(COLON, tValue, currentLine_));
			case ';':
				currentPos_++;
				tValue = ";";
				return (cToken(SEMICOLON, tValue, currentLine_));
			case '\n':
				currentLine_++;
				currentPos_++;
				continue; // skip to the next character
			default:
				break;
		}
		if (isdigit(*currentPos_)) {
			while (currentPos_ != allConfig_.end() && *currentPos_ != '\n' && (isdigit(*currentPos_) || *currentPos_ == '.')) {
				tValue += *currentPos_;
				currentPos_++;			}
			return (cToken(NUMBER, tValue, currentLine_));
		}
		else if (isalpha(*currentPos_) || *currentPos_ == '/') {
			while (currentPos_ != allConfig_.end() && *currentPos_ != '\n' && *currentPos_ != ' ' && *currentPos_ != ';' && isprint(*currentPos_) ) {
				tValue.push_back(*currentPos_);
				currentPos_++;
			}
			return (cToken(STRING, tValue, currentLine_));
		}
		else {
			while (currentPos_ != allConfig_.end() && *currentPos_ != '\n') {
				tValue += *currentPos_;
				currentPos_++;
			}
			return (cToken(UNKNOWN, tValue, currentLine_));
		}
	}
		if (currentPos_ == allConfig_.end()) {
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
	}
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
			}
		}
	}
	if (braceCount > 0) {
		throw ConfParser::ConfParserException("unmatched opening brace ", openBraceLine);
	}
}

void	ConfParser::setAllTokens() {
	
	cToken	currentToken;
	while (currentPos_ < allConfig_.end()) {
		currentToken = defineToken();
		if (currentToken.type == UNKNOWN) {
			throw ConfParser::ConfParserException("unknown token: ", currentToken.value, currentToken.line);
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
}

void	ConfParser::semicolonCheck(const tokenType& type, size_t line) {
	if (type != SEMICOLON) {
		throw ConfParser::ConfParserException("expected ';' after the value ", line);
	}
}

void	ConfParser::validateIP(const std::string& ip, size_t line) {

	int	dotCount(0);
	for (const auto& c : ip) {
		if (c == '.')
			dotCount++;
	}
	if (dotCount != 3) {
		throw ConfParserException("invalid IP address: ", ip, line);
	}
	
	std::string	ipPart;
	size_t	start(0);
	size_t	end(0);
	for (int i(0); i < 4; i++) {
		if (i < 3) {
			end = ip.find('.', start);
			if (end == std::string::npos)
				throw ConfParserException("invalid IP address: ", ip, line);
			ipPart = ip.substr(start, end - start);
		}
		else {
			ipPart = ip.substr(start);
		}
		
		if (ipPart.empty() || ipPart.find_first_not_of("0123456789") != std::string::npos) {
			throw ConfParserException("invalid IP address: ", ip, line);
		
		}
		
		int ipPartInt = std::stoi(ipPart);
		if (ipPartInt < 0 || ipPartInt > 255) {
			throw ConfParserException("invalid IP address: ", ip, line);
		}
		start = end + 1;
	}
}

void	ConfParser::addIP(SingleServer& newServer, size_t& i) {
	i++; //move to the token after listen
	
	if (allTokens_[i].type != NUMBER && allTokens_[i].value != "localhost") {
		throw ConfParserException("expected IP/localhost and port number after 'listen'", allTokens_[i].line);
	}
	
	if (allTokens_[i].value == "localhost") {
		newServer.setServIP("127.0.0.1");
	}
	else {
		validateIP(allTokens_[i].value, allTokens_[i].line);
		newServer.setServIP(allTokens_[i].value);
	}
	i++; // moved to the semicolon?
	semicolonCheck(allTokens_[i].type, allTokens_[i].line);
}

void	ConfParser::validatePort(const std::string& port, size_t line) {
	
	int	portInt(std::stoi(port));
	if (portInt < 1024 || portInt > 65535)
		throw ConfParserException("invalid port number: ",  port, line);
}

/// @brief checks and sets the port using only the 1st one in the file
/// @param newServer 
/// @param i 
/// @return 
void	ConfParser::addPort(SingleServer& newServer, size_t& i) {
	
	i++; //moved to the token after "port"
	if (allTokens_[i].type != NUMBER) {
		throw ConfParserException("expected port number after 'port'", allTokens_[i].line);
	}
	validatePort(allTokens_[i].value, allTokens_[i].line);
	if (newServer.getServPortInt() == -1) {
		newServer.setServPortString(allTokens_[i].value);
		newServer.setServPortInt(std::stoi(allTokens_[i].value));
	}
	i++; //moved to the semicolon?
	semicolonCheck(allTokens_[i].type, allTokens_[i].line);
}

void	ConfParser::addServerName(SingleServer& newServer, size_t& i) {
	i++; //moving to the token after 'server_name'
	if (allTokens_[i].type != STRING) {
		throw ConfParserException("expected string after server_name directive", allTokens_[i].line);
	}
	newServer.setServName(allTokens_[i].value);
	i++; //moved to the semicolon?
	semicolonCheck(allTokens_[i].type, allTokens_[i].line);
}

void	ConfParser::addLocation(SingleServer& server, size_t& i) {
	i++; // move to the location's path
	if (allTokens_[i].type != STRING) {
		throw ConfParserException("expected location's path after 'location' directive", allTokens_[i].line);
	}
	Location newLocation;
	newLocation.setPath(allTokens_[i].value);
	i++; // move to the opening brace
	if (allTokens_[i].type != OPEN_BRACE) {
		throw ConfParserException("expected '{' after location's path", allTokens_[i].line);
	}
	i++; //move to the next token
	while (allTokens_[i].type != CLOSE_BRACE) {
		if (allTokens_[i].value == "root") {
			i++; // move to the next value
			if (allTokens_[i].type != STRING) {
				throw ConfParserException("expected a string value after root directive", allTokens_[i].line);
			}
			newLocation.setRoot(allTokens_[i].value);
			i++; //move to the semicolon?
			semicolonCheck(allTokens_[i].type, allTokens_[i].line);
		}
		else if (allTokens_[i].value == "index") {
			i++; //move to the index value
			if (allTokens_[i].type != STRING) {
				throw ConfParserException("expected a string value after index directive",allTokens_[i].line);
			}
			newLocation.setIndex(allTokens_[i].value);
			i++; //move to the semicolon?
			semicolonCheck(allTokens_[i].type, allTokens_[i].line);
		}
		else if (allTokens_[i].value == "allowed_methods") {
			i++;
			if (allTokens_[i].type != STRING) {
				throw ConfParserException("expected a string value after index directive", allTokens_[i].line);
			}
			while (allTokens_[i].type == STRING) {
				if (allTokens_[i].value != "GET" && allTokens_[i].value != "POST" && allTokens_[i].value != "DELETE") {
					throw ConfParserException("expected one of valid allowed_methods (GET, POST or DELETE)", allTokens_[i].line);
				}
				newLocation.setAllowedMethods(allTokens_[i].value);
				i++; //move to the semicolon?
			}
			semicolonCheck(allTokens_[i].type, allTokens_[i].line);
		}
		else if (allTokens_[i].value == "autoindex") {
			i++;
			if (allTokens_[i].type != STRING) {
				throw ConfParserException("expected a string value after index directive", allTokens_[i].line);
			}
			if (allTokens_[i].value == "on") {
				newLocation.setAutoindex(true);
			}
			else if (allTokens_[i].value == "off") {
				newLocation.setAutoindex(false);
			}
			else {
				throw ConfParserException("expected 'on' or 'off' after autoindex directive", allTokens_[i].line);
			}
			i++; //move to the semicolon?
			semicolonCheck(allTokens_[i].type, allTokens_[i].line);
		}
		else if (allTokens_[i].value == "return") {
			i++; //move to the redirect code
			if (allTokens_[i].type != NUMBER || (allTokens_[i].value != "301" && allTokens_[i].value != "302")) {
				throw ConfParserException("expected the redirection code 301 or 302 after 'return'", allTokens_[i].line);
			}
			newLocation.setRedirectionCode(std::stoi(allTokens_[i].value));
			i++; //move to the redirect path
			if (allTokens_[i].type != STRING) {
				throw ConfParserException("exptected the redirection path after the redirection code", allTokens_[i].line);
			}
			newLocation.setRedirectionsPath(allTokens_[i].value);
			i++; //move to the semicolon
			semicolonCheck(allTokens_[i].type, allTokens_[i].line);
		}
		else if (allTokens_[i].value == "upload_path") {
			i++; //move to the upload path
			if (allTokens_[i].type != STRING) {
				throw ConfParserException("expected a string after 'upload_path directive", allTokens_[i].line);
			}
			newLocation.setUploadPath(allTokens_[i].value);
			i++; //move to the semicolon
			semicolonCheck(allTokens_[i].type, allTokens_[i].line);
		}
		i++; //move after the semicolon
		//TODO add cgi-related stuff
	}
	if (allTokens_[i].type != CLOSE_BRACE) {
		throw ConfParserException("expected '}' at the end of the location block", allTokens_[i].line);
	}
	std::cout << newLocation << std::endl;
	server.setLocations(newLocation);
}

void	ConfParser::addMaxBodySize(SingleServer& newServer, size_t& i) {
	i++; //move to the next token
	if (allTokens_[i].type != NUMBER) {
		throw ConfParserException("expected a number after 'client_max_body_size' directive", allTokens_[i].line);
	}
	if (allTokens_[i].value.length() > 8) {
		throw ConfParserException("'client_max_body_size' value is too big", allTokens_[i].line);
	}
	int	maxSize = std::stoi(allTokens_[i].value);
	if (maxSize < 0 || maxSize > 10485760) {
		throw ConfParserException("'client_max_body_size' value can be between 1 - 10485760", allTokens_[i].line);
	}
	newServer.setMaxBodySize(maxSize);
	i++; //moved to the semicolon?
	semicolonCheck(allTokens_[i].type, allTokens_[i].line);
}

void	ConfParser::addErrorPages(SingleServer& newServer, size_t& i) {
	i++; // move to the next token
	if (allTokens_[i].type != NUMBER && allTokens_[i].value.length() != 3) {
		//TODO change it according to our error pages
		throw ConfParserException("expected error page code value after error_page directive", allTokens_[i].line);
	}
	if (allTokens_[i + 1].type != STRING) {
		throw ConfParserException("expected string after error_page code directive", allTokens_[i].line);
	}
	int	errorCode = std::stoi(allTokens_[i].value);
	i++; // move to the error page path
	newServer.setErrorPages(errorCode, allTokens_[i].value);;
	i++; //moved to the semicolon?
	semicolonCheck(allTokens_[i].type, allTokens_[i].line);
}


/// @brief in the server block, goes from '{' until '}' and populates the single server and adds it in vector with servers
/// @param servers vector containing all servers
/// @param i current token's position
/// @return failure or success
void	ConfParser::populateServers(Server42& servers, size_t& i) {
	
	i++; //moving to the open brace token
	if (allTokens_[i].type != OPEN_BRACE) {
		throw ConfParserException("expected '{' after 'server' directive ", allTokens_[i].line);
	}
	i++; // go to the next token after the opening brace
	SingleServer	newServer;

	while (i < allTokens_.size() && allTokens_[i].value != "server") {
		if (allTokens_[i].type == DIRECTIVE) {
			//all the adding functions will end with the semicolon token

			if (allTokens_[i].value == "listen") {
				addIP(newServer, i);
			}
			else if (allTokens_[i].value == "port") {
				addPort(newServer, i);
			}
			else if (allTokens_[i].value == "server_name") {
				addServerName(newServer, i);
			}
			else if (allTokens_[i].value == "location") {
				addLocation(newServer, i);
			}
			else if (allTokens_[i].value == "client_max_body_size") {
				addMaxBodySize(newServer, i);
			}
			else if (allTokens_[i].value == "error_page") {
				addErrorPages(newServer, i);
			}
			else {
				throw ConfParserException("unknown directive: ", allTokens_[i].value, allTokens_[i].line);
			}
			i++; //move to the token after the semicolon
		}
		else if (allTokens_[i].type == CLOSE_BRACE) {
			i++; //move to the next token after the closing brace
			servers.addServer(newServer);
			return ;
		}
		else {
			throw ConfParserException("unexpected token: ", allTokens_[i].value, allTokens_[i].line);
		}
	}
	throw ConfParserException("unexpected token: ", allTokens_[i].value, allTokens_[i].line);
}

void ConfParser::parseConfig(Server42& servers) {
	
	try {
		setAllTokens();
	}
	catch (const ConfParserException& e) {
		throw std::runtime_error(e.what());
	}
		
	size_t i(0);
	while (allTokens_[i].type != END_OF_FILE)
	{
		try {
			if (allTokens_[i].type == DIRECTIVE && allTokens_[i].value == "server") {
				populateServers(servers, i);
				continue;
			}
			else {
				throw ConfParserException("unexpected token: ", allTokens_[i].value, allTokens_[i].line);
			}
		}
		catch (const ConfParserException& e) {
			throw std::runtime_error(e.what());
		}
		i++;
	}
}
