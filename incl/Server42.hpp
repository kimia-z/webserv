/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   Server42.hpp                                       :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/05/05 10:37:22 by mstencel      #+#    #+#                 */
/*   Updated: 2025/05/13 13:49:07 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER42
# define SERVER42

#include "Webserv42.hpp"

#include <vector>
#include <unordered_map>
#include <iostream>

class Server42 {

	public:
		Server42();
		Server42(const Server42& copy);
		Server42& operator=(const Server42& copy);
		~Server42();

		// getters:
		
		
		
		//setters:
		

		
	
	private:
		std::vector<SingleServer>				servers_;	
};

#endif