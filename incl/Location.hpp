/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   Location.hpp                                       :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/06/24 09:50:51 by mstencel      #+#    #+#                 */
/*   Updated: 2025/06/24 10:00:08 by mstencel      ########   odam.nl         */
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

		void	setPath(const std::string& path);
		void	setRoot(const std::string& root);
		void	setIndex(const std::string& index);
		void	setAllowedMethods(const std::string& method);
		void	setAutoindex(bool autoindex);
		void	setUploadPath(const std::string& uploadPath);
		void	setErrorPages(const int& errorCode, const std::string& errorPage);
		void	setRedirections(const std::string& redirections);

		std::string								getPath() const;
		std::string								getRoot() const;
		std::string								getIndex() const;
		std::vector<std::string>				getAllowedMethods() const;
		bool									getAutoindex() const;
		std::string								getUploadPath() const;
		std::unordered_map<int, std::string>	getErrorPages() const;
		std::string								getRedirections() const;
		
	private:
		std::string								path_; //location of the directory
		std::string								root_; // path to the folder with sites
		std::string								index_; // index file (index.html)
		std::vector<std::string>				allowedMethods_; //methods that can be used
		bool									autoindex_; //true/false of autoindex
		std::string								uploadPath_; //path with the uploads' folder
		std::unordered_map<int, std::string>	errorPages_; //all error pages, key = error number & value = page path
		std::string								redirections_; //if applicable, redirection path
};
