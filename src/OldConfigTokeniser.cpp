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

#include "../incl/OldConfigTokeniser.hpp"

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
	std::cout << "currentPos_ in set: " << *currentPos_ << std::endl;
}

void	ConfTokeniser::skipWhiteSpaceComments() {
	// std::cout << "here" << std::endl;
	std::cout << "in skip current: " << *currentPos_ << std::endl;
	// int i = 0;
	for (; currentPos_ != allConfig_.end(); currentPos_++) {
		std::cout << "0currentPos_: " << *currentPos_ << std::endl;
		if (isspace(*currentPos_)) {
			continue;
		}
		else if (*currentPos_ == '#') {
			std::cout << "in comment" << std::endl;
			while (currentPos_ != allConfig_.end() && *currentPos_ != '\n') {
				*currentPos_++;
				// i++;
				// std::cout << "in skipping currently: " << i << std::endl;
			}
		}
		else
			break;
	}
	// std::cout << " in skipping currently: " << i << std::endl;
}

/// @brief checks what type is the token (see the tokenType enum)
/// @return created token
cToken	ConfTokeniser::defineToken() {
	int i = 0;
	while (currentPos_ != allConfig_.end()) {
		std::cout << "out of skip current: " << *currentPos_ << std::endl;
	// for (int i = 0; i < 10; i++) {
		// std::cout << "now here: " << &currentPos_ << std::endl;
		// std::cout << "line: " << currentLine_ << std::endl;
		skipWhiteSpaceComments();
		// std::cout << "after here: " << &currentPos_ << std::endl;
		if (currentPos_ == allConfig_.end()) {
			std::cout << "EOF found" << std::endl;
			return (cToken(END_OF_FILE, ""));
		}
		if (*currentPos_ == '{') {
			std::cout << "1currentPos_: " << *currentPos_ << std::endl;
			std::cout << "1currently: " << i << std::endl;
			currentPos_++;
			i++;
			std::cout << "{ found" << std::endl;
			return (cToken(OPEN_BRACE, "{"));
		}
		if (*currentPos_ == '}') {
			std::cout << "2currentPos_: " << *currentPos_ << std::endl;
			std::cout << "2currently: " << i << std::endl;
			currentPos_++;
			i++;
			std::cout << "} found" << std::endl;
			return (cToken(CLOSE_BRACE, "}"));
		}
		if (*currentPos_ == ':') {
			std::cout << "3currentPos_: " << *currentPos_ << std::endl;
			std::cout << "3currently: " << i << std::endl;
			currentPos_++;
			i++;
			std::cout << ": found" << std::endl;
			return (cToken(COLON, ":"));
		}
		if (*currentPos_ == ';') {
			std::cout << "4currentPos_: " << *currentPos_ << std::endl;
			std::cout << "4currently: " << i << std::endl;
			currentPos_++;
			i++;
			std::cout << "; found" << std::endl;
			return (cToken(SEMICOLON, ";"));
		}
		if (*currentPos_ == '/') {
			std::cout << "5currentPos_: " << *currentPos_ << std::endl;
			std::cout << "5currently: " << i << std::endl;
			currentPos_++;
			i++;
			std::cout << "/ found" << std::endl;
			return (cToken(SLASH, "/"));
		}
		std::string	tValue;
		if (isdigit(*currentPos_)) {
			while (currentPos_ != allConfig_.end() && *currentPos_ != '\n' && (isdigit(*currentPos_) || *currentPos_ == '.')) {
				tValue += *currentPos_;
				std::cout << "6currentPos_: " << *currentPos_ << std::endl;
				std::cout << "6currently: " << i << std::endl;
				currentPos_++;
				i++;
			}
			std::cout << tValue << " found" << std::endl;
			return (cToken(NUMBER, tValue));
		}
		if (isalpha(*currentPos_)) {
			while (currentPos_ != allConfig_.end() && *currentPos_ != '\n' && (isalpha(*currentPos_) || isdigit(*currentPos_) || *currentPos_ == '-' || *currentPos_ == '_' || *currentPos_ == '.')) {
				tValue.push_back(*currentPos_);
				std::cout << "7currentPos_: " << &currentPos_ << std::endl;
				std::cout << "7currently: " << i << std::endl;
				currentPos_++;
				i++;
			}
			std::cout << tValue << " found???" << std::endl;
			return (cToken(STRING, tValue));
		}
		if (*currentPos_ == '\n') {
			currentLine_++;
			std::cout << "8currentPos_: " << &currentPos_ << std::endl;
			std::cout << "8currently: " << i << std::endl;
			currentPos_++;
			i++;
			std::cout << "\\n found" << std::endl;
		}
		// std::cout << "currently: " << i << std::endl;
	}
	std::string tValue;
	while (currentPos_ != allConfig_.end() && *currentPos_ != '\n') {
		tValue += *currentPos_;
		currentPos_++;
		std::cout << "9currentPos_: " << *currentPos_ << std::endl;
		std::cout << "9currently: " << i << std::endl;
		i++;
	}
	std::cout << "Unknown input: " << tValue << " found" << std::endl;
	// std::cout << "at the end currently: " << i << std::endl;
	return (cToken(UNKNOWN, tValue));
}
