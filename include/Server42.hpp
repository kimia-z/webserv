/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   Server42.hpp                                       :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/05/05 10:37:22 by mstencel      #+#    #+#                 */
/*   Updated: 2025/05/06 15:57:56 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER42
# define SERVER42

#include "Webserv42.hpp"

class Server42 {

	public:
		Server42();
		Server42(const Server42& copy);
		Server42& operator=(const Server42& copy);
		~Server42();
	
	private:
		std::string								serverHost_;
		int										serverPort_;
		std::string								serverName_;
		std::unordered_map<int, std::string>	errorPages_;
		
		
		
};

#endif