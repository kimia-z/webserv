/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ConfigTokeniser.cpp                                :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/05/20 11:28:02 by mstencel      #+#    #+#                 */
/*   Updated: 2025/05/20 13:37:34 by mstencel      ########   odam.nl         */
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



