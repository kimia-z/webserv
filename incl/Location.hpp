/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   Location.hpp                                       :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/06/24 09:50:51 by mstencel      #+#    #+#                 */
/*   Updated: 2025/06/27 14:12:31 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <vector>
#include <unordered_map>


class Location {

	public:
		Location();
		Location(const Location& copy);
		Location& operator=(const Location& copy);
		~Location();

		void	setPath(const std::string& newPath);
		void	setRoot(const std::string& newRoot);
		void	setIndex(const std::string& newIndex);
		void	setAllowedMethods(const std::string& newMethod);
		void	setAutoindex(bool newAutoindex);
		void	setUploadPath(const std::string& newUploadPath);
		// void	setErrorPages(const int& newErrorCode, const std::string& newErrorPage);
		void	setRedirections(const std::string& newRedirections);

		std::string								getPath() const;
		std::string								getRoot() const;
		std::string								getIndex() const;
		std::vector<std::string>				getAllowedMethods() const;
		bool									getAutoindex() const;
		std::string								getUploadPath() const;
		// std::unordered_map<int, std::string>	getErrorPages() const;
		std::string								getRedirections() const;
		
	private:
		std::string								path_; //location of the directory
		std::string								root_; // path to the folder with sites
		std::string								index_; // index file (index.html)
		std::vector<std::string>				allowedMethods_; //methods that can be used
		bool									autoindex_; //true/false of autoindex
		std::string								uploadPath_; //path with the uploads' folder
		std::string								redirections_; //if applicable, redirection path
};
