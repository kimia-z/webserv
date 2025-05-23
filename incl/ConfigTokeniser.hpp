/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ConfigTokeniser.hpp                                :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/05/20 11:28:39 by mstencel      #+#    #+#                 */
/*   Updated: 2025/05/23 15:13:12 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#ifndef	CONFIGTOKENISER_HPP
# define CONFIGTOKENISER_HPP

#include <iostream>
#include <string> //for std::string::iterator

enum	tokenType {
	OPEN_BRACE, // {
	CLOSE_BRACE, // }
	NUMBER, // numbers (includes the dot)
	STRING, // words (includes the dot)
	COLON, // :
	SEMICOLON, // ;
	SLASH, // /
	END_OF_FILE, // end of the file
	UNKNOWN // anything else
};


struct configToken {
	tokenType	type; //token's type
	std::string	value; //token's value
	
	configToken(tokenType t, std::string v) : type(t), value(v) {}	//constructor -> already sets type & value while creating the token
};

class	ConfTokeniser {
	public:
	ConfTokeniser();
	ConfTokeniser(const std::string& config); //constructor that will use the config file data
	~ConfTokeniser();
	ConfTokeniser(const ConfTokeniser& copy);
	ConfTokeniser& operator=(const ConfTokeniser& copy);

	std::string				getAllConfig() const;
	std::string::iterator	getCurrentPos()	const;
	
	void	setAllConfig(const std::string& config);
	void	setCurrentPos(std::string::iterator pos);

	void		skipWhiteSpaceComments(); //skips white spaces & comments
	configToken	defineToken(); //returns defined token
	
	private:
	std::string				allConfig_; //config file saved as a string
	std::string::iterator	currentPos_; //current position in the saved string
};

#endif