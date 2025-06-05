/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ConfigParser.hpp                                   :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/05/20 12:42:16 by mstencel      #+#    #+#                 */
/*   Updated: 2025/05/26 11:15:08 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#ifndef	CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

#include <iostream>
#include "ConfigTokeniser.hpp"
#include "SingleServer.hpp"

class ConfigParser {
	public:
		ConfigParser();
		ConfigParser(std::stringstream& buffer);
		ConfigParser(const ConfigParser& copy);
		ConfigParser& operator=(const ConfigParser& copy);
		~ConfigParser();
	
		ConfTokeniser	getTokeniser() const;
		cToken			getCurrentToken() const;
		
		void			setTokeniser(const ConfTokeniser& tokeniser);
		void			setCurrentToken(const cToken& token);
		
		int				parseConfig(std::vector<SingleServer>& servers);
		int				parseServerBlock(std::vector<SingleServer>& servers);
		int				configError(const std::string& errorMessage) const;

	
	private:
		ConfTokeniser	tokeniser_;
		cToken			currentToken_;
};

#endif