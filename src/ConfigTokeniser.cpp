/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   cTokeniser.cpp                                :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/05/20 11:28:02 by mstencel      #+#    #+#                 */
/*   Updated: 2025/05/26 10:38:31 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../incl/ConfigTokeniser.hpp"

//canonical form

ConfTokeniser::ConfTokeniser(): allConfig_(""), currentPos_(allConfig_.begin()), currentLine_(1) {
	std::cout << "ConfTokeniser default constructor called" << std::endl;
}

ConfTokeniser::ConfTokeniser(const std::string& config) {
	allConfig_ = config;
	currentPos_ = allConfig_.begin();
	currentLine_ = 1;
	std::cout << "ConfTokeniser constructor with param called" << std::endl;
}

ConfTokeniser::ConfTokeniser(const ConfTokeniser& copy): 
	allConfig_(copy.allConfig_),
	currentPos_(copy.currentPos_),
	currentLine_(copy.currentLine_) {
	std::cout << "ConfTokeniser copy constructor called" << std::endl;
}

ConfTokeniser&	ConfTokeniser::operator=(const ConfTokeniser& copy) {
	if (this != &copy) {
		allConfig_ = copy.allConfig_;
		currentPos_ = copy.currentPos_;
		currentLine_ = copy.currentLine_;
	}
	std::cout << "ConfTokeniser copy assignment operator called" << std::endl;
	return (*this);
}

ConfTokeniser::~ConfTokeniser() {
	std::cout << "ConfTokeniser default destructor called" << std::endl;
}

//getters
std::string	ConfTokeniser::getAllConfig() const {
	return (allConfig_);
}

std::string::iterator	ConfTokeniser::getCurrentPos() const {
	return (currentPos_);
}

//setters
void	ConfTokeniser::setAllConfig(const std::string& config) {
	allConfig_ = config;
}

void	ConfTokeniser::setCurrentPos(std::string::iterator pos) {
	currentPos_ = pos;
}

void	ConfTokeniser::skipWhiteSpaceComments() {
	for (; currentPos_ != allConfig_.end(); currentPos_++) {
		if (isspace(*currentPos_))
			continue;
		else if (*currentPos_ == '#') {
			while (currentPos_ != allConfig_.end() && *currentPos_ != '\n')
				currentPos_++;
		}
		else
			break;
	}
}

/// @brief checks what type is the token (see the tokenType enum)
/// @return created token
cToken	ConfTokeniser::defineToken() {
	
	while (currentPos_ != allConfig_.end()) {
		skipWhiteSpaceComments();
		if (currentPos_ == allConfig_.end()) {
			std::cout << "EOF found" << std::endl;
			return (cToken(END_OF_FILE, ""));
		}
		if (*currentPos_ == '{') {
			currentPos_++;
			std::cout << "{ foung" << std::endl;
			return (cToken(OPEN_BRACE, "{"));
		}
		if (*currentPos_ == '}') {
			currentPos_++;
			std::cout << "} found" << std::endl;
			return (cToken(CLOSE_BRACE, "}"));
		}
		if (*currentPos_ == ':') {
			currentPos_++;
			std::cout << ": found" << std::endl;
			return (cToken(COLON, ":"));
		}
		if (*currentPos_ == ';') {
			currentPos_++;
			std::cout << "; found" << std::endl;
			return (cToken(SEMICOLON, ";"));
		}
		if (*currentPos_ == '/') {
			currentPos_++;
			std::cout << "/ found" << std::endl;
			return (cToken(SLASH, "/"));
		}
		std::string	tValue;
		if (isdigit(*currentPos_)) {
			while (currentPos_ != allConfig_.end() && *currentPos_ != '\n' && (isdigit(*currentPos_) || *currentPos_ == '.')) {
				tValue += *currentPos_;
				currentPos_++;
			}
			std::cout << tValue << " found" << std::endl;
			return (cToken(NUMBER, tValue));
		}
		if (isalpha(*currentPos_)) {
			while (currentPos_ != allConfig_.end() && *currentPos_ != '\n' && (isalpha(*currentPos_) || isdigit(*currentPos_) || *currentPos_ == '-' || *currentPos_ == '_' || *currentPos_ == '.')) {
				tValue += *currentPos_;
				currentPos_++;
			}
			std::cout << tValue << " found" << std::endl;
			return (cToken(STRING, tValue));
		}
		if (*currentPos_ == '\n') {
			currentLine_++;
			currentPos_++;
			std::cout << "\\n found" << std::endl;
		}
	}
	std::string tValue;
	while (currentPos_ != allConfig_.end() && *currentPos_ != '\n') {
		tValue += *currentPos_;
		currentPos_++;
	}
	std::cout << "Unknown input: " << tValue << " found" << std::endl;
	return (cToken(UNKNOWN, tValue));
}
