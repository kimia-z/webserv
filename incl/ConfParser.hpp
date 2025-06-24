/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   ConfParser.hpp                                     :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/06/12 11:28:20 by mstencel      #+#    #+#                 */
/*   Updated: 2025/06/24 09:45:42 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFPARSER_HPP
# define CONFPARSER_HPP

#include <iostream>
#include <string> //for std::string::iterator


// #define LISTEN 0
// #define PORT 1
// #define SERVER_NAME 2
// #define LOCATION 3
// #define LOC_ROOT 4
// #define LOC_INDEX 5
// #define LOC_METHODS 6
// #define LOC_AUTOINDEX 7
// #define CLIENT_MAX_BODY_SIZE 8
// #define ERROR_PAGE 9

#include "Server42.hpp"
class Server42;

#include "SingleServer.hpp"
class SingleServer;

enum	tokenType {
	OPEN_BRACE, // {
	CLOSE_BRACE, // }
	NUMBER, // numbers (includes the dot)
	STRING, // words (includes the dot)
	COLON, // :
	SEMICOLON, // ;
	SLASH, // /
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
	

		// std::string				getAllConfig() const;
		// std::string::iterator	getCurrentPos() const;
		int					getCurrentLine() const;
		std::vector<cToken>	getAllTokens() const;

		void				setAllConfig(const std::string& config);
		int					setAllTokens();
		// void				setCurrentPos(const std::string::iterator pos);
		
		cToken				defineToken(); //returns defined token
		void				skipWhiteSpaceComment(); //skips white spaces & comments
		int					tokensCheck(); // runs basic checks on the tokens
		int					semicolonCheck(const tokenType& type, size_t line); //checks if each line is closed with semicolon

		int					validateIP(const std::string& ip, size_t line); //validates the IP address
		int					validatePort(const std::string& port, size_t line); //validates the port number

		int					parseConfig(Server42& servers);
		int					populateServers(Server42& servers, size_t& i);
		int					addIP(SingleServer& newServer, size_t& i);
		int					addPort(SingleServer& newServer, size_t& i);
		int					addServerName(SingleServer& newServer, size_t& i);
		
		int					populateLocation(SingleServer& server, size_t& i);
		
		int					parseServerBlock(std::vector<SingleServer>& server);

	private:
		std::string				allConfig_; // config file in a string format
		std::string::iterator	currentPos_; // current position in the allConfig string
		int						currentLine_; // current line number for error handling
		std::vector<cToken>		allTokens_; // vector with all tokens
};

#endif