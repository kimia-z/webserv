/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ConfParser.hpp                                     :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/06/12 11:28:20 by mstencel      #+#    #+#                 */
/*   Updated: 2025/07/07 15:10:07 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFPARSER_HPP
# define CONFPARSER_HPP

#include <iostream>
#include <string> //for std::string::iterator

#include "Server42.hpp"
class Server42;

#include "SingleServer.hpp"
class SingleServer;

enum	tokenType {
	OPEN_BRACE, // {
	CLOSE_BRACE, // }
	NUMBER, // numbers (includes the dot)
	STRING, // words (includes the dot)
	DIRECTIVE, // directives, e.g. listen, error_pages, port, etc
	COLON, // :
	SEMICOLON, // ;
	END_OF_FILE, // end of the file
	UNKNOWN, // anything else
	EMPTY // for initialisation
};

struct cToken {
	tokenType	type; //token's type
	std::string	value; //token's value
	size_t		line; //line in the file where the token was found
	
	cToken() : type(EMPTY), value(""), line(1) {} //default constructor
	cToken(tokenType t, std::string& v, size_t l) : type(t), value(v), line(l) {}	//constructor -> already sets type & value while creating the token
};

class	ConfParser {
	
	public:
		ConfParser();
		ConfParser(const std::string& config);
		ConfParser(const ConfParser& copy);
		ConfParser& operator=(const ConfParser& copy);
		~ConfParser();
	

		int					parseConfig(Server42& servers);

		int					getCurrentLine() const;
		std::vector<cToken>	getAllTokens() const;

		void				setAllConfig(const std::string& config);
		void				setAllTokens();
		
		cToken				defineToken(); //returns defined token
		void				skipWhiteSpaceComment(); //skips white spaces & comments

		int					semicolonCheck(const tokenType& type, size_t line); //checks if each line is closed with semicolon
		void				validateBraces(); // runs the check on number of braces
		int					validateIP(const std::string& ip, size_t line); //validates the IP address
		int					validatePort(const std::string& port, size_t line); //validates the port number

		int					populateServers(Server42& servers, size_t& i);
		int					addIP(SingleServer& newServer, size_t& i);
		int					addPort(SingleServer& newServer, size_t& i);
		int					addServerName(SingleServer& newServer, size_t& i);
		int					addLocation(SingleServer& server, size_t& i);
		int					addMaxBodySize(SingleServer& newServer, size_t& i);
		int					addErrorPages(SingleServer& newServer, size_t& i);


	private:
		std::string				allConfig_; // config file in a string format
		std::string::iterator	currentPos_; // current position in the allConfig string
		int						currentLine_; // current line number for error handling
		std::vector<cToken>		allTokens_; // vector with all tokens

	
	class	ConfParserException : public std::exception {
		public:
			ConfParserException(const std::string& message, const std::string& token, size_t line);
			ConfParserException(const std::string& message, size_t line);
			ConfParserException(const std::string& message);
			const char* what() const throw();
			~ConfParserException();
			
		private:
			std::string allMessage_;
	};
		
};

#endif