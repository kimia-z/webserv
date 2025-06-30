/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   Location.cpp                                       :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/06/27 13:59:39 by mstencel      #+#    #+#                 */
/*   Updated: 2025/06/27 14:25:03 by mstencel      ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "../incl/Location.hpp"

Location::Location() : 
	path_(""),
	root_(""),
	index_(""),
	allowedMethods_(),
	autoindex_(false),
	uploadPath_(""),
	// errorPages_(),
	redirections_("") {
	// std::cout << "Location's constructor was called" << std::endl;
}

Location::Location(const Location& copy) : 
	path_(copy.path_),
	root_(copy.root_),
	index_(copy.index_),
	allowedMethods_(copy.allowedMethods_),
	autoindex_(copy.autoindex_),
	uploadPath_(copy.uploadPath_),
	// errorPages_(copy.errorPages_),
	redirections_(copy.redirections_) {
	// std::cout << "Location's copy constructor was called" << std::endl;
}

Location& Location::operator=(const Location& copy) {
	if (this != &copy) {
		path_ = copy.path_;
		root_ = copy.root_;
		index_ = copy.index_;
		allowedMethods_ = copy.allowedMethods_;
		autoindex_ = copy.autoindex_;
		uploadPath_ = copy.uploadPath_;
		// errorPages_ = copy.errorPages_;
		redirections_ = copy.redirections_;
	}
	// std::cout << "Location's copy operator was called" << std::endl;
	return (*this);
}

Location::~Location() {
	// std::cout << "Location's  destructor was called" << std::endl;
}

std::string Location::getPath() const {
	return (path_);
}

std::string	Location::getRoot() const{
	return (root_);
}

std::string	Location::getIndex() const {
	return (index_);
}

std::vector<std::string>	Location::getAllowedMethods() const {
	return (allowedMethods_);
}

bool	Location::getAutoindex() const {
	return (autoindex_);
}

std::string		Location::getUploadPath() const {
	return (uploadPath_);
}

// std::unordered_map<int, std::string>	Location::getErrorPages() const {
// 	return (errorPages_);
// }

std::string	Location::getRedirections() const {
	return (redirections_);
}


void	Location::setPath(const std::string& newPath) {
	path_ = newPath;
}

void	Location::setRoot(const std::string& newRoot) {
	root_ = newRoot;
}

void	Location::setIndex(const std::string& newIndex) {
	index_ = newIndex;
}

void	Location::setAllowedMethods(const std::string& newMethod) {
	allowedMethods_.push_back(newMethod);
}

void	Location::setAutoindex(bool newAutoindex) {
	autoindex_ = newAutoindex;
}

void	Location::setUploadPath(const std::string& newUploadPath) {
	uploadPath_ = newUploadPath;
}

void	Location::setRedirections(const std::string& newRedirections) {
	redirections_ = newRedirections;
}


