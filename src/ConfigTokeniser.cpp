/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ConfigTokeniser.cpp                                :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/05/20 11:28:02 by mstencel      #+#    #+#                 */
/*   Updated: 2025/05/20 15:39:37 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../incl/ConfigTokeniser.hpp"

//canonical form

ConfTokeniser::ConfTokeniser() {
	std::cout << "ConfToekniser default constructor called" << std::endl;
}

ConfTokeniser::ConfTokeniser(const std::string & config) {
	allConfig_ = config;
	currentPos_ = allConfig_.begin();
	std::cout << "ConfTokeniser constructor with param called" << std::endl;
}

ConfTokeniser::ConfTokeniser(const ConfTokeniser& copy): 
	allConfig_(copy.allConfig_),
	currentPos_(copy.currentPos_) {
	std::cout << "ConfTokeniser copy constructor called" << std::endl;
}

ConfTokeniser&	ConfTokeniser::operator=(const ConfTokeniser& copy) {
	if (this != &copy) {
		allConfig_ = copy.allConfig_;
		currentPos_ = copy.currentPos_;
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

configToken	ConfTokeniser::defineToken() {
	
	skipWhiteSpaceComments();
	if (currentPos_ == allConfig_.end())
	return (configToken(END_OF_FILE, ""));
	if (*currentPos_ == '{') {
		currentPos_++;
		return (configToken(OPEN_BRACE, "{"));
	}
	if (*currentPos_ == '}') {
		currentPos_++;
		return (configToken(CLOSE_BRACE, "}"));
	}
	if (*currentPos_ == ':') {
		currentPos_++;
		return (configToken(COLON, ":"));
	}
	if (*currentPos_ == ';') {
		currentPos_++;
		return (configToken(SEMICOLON, ";"));
	}
	std::string	tValue;
	if (isdigit(*currentPos_)) {
		while (currentPos_ != allConfig_.end() && isdigit(*currentPos_)) {
			tValue += *currentPos_;
			currentPos_++;
		}
		return (configToken(NUMBER, tValue));
	}
	if (isalpha(*currentPos_)) {
		while (currentPos_ != allConfig_.end() && (isalpha(*currentPos_) || isdigit(*currentPos_) || *currentPos_ == '-' || *currentPos_ == '_')) {
			tValue += *currentPos_;
			currentPos_++;
		}
		return (configToken(STRING, tValue));
	}
}