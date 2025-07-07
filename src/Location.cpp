/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   Location.cpp                                       :+:    :+:            */
/*                                                     +:+                    */
/*   By: mstencel <mstencel@student.codam.nl>         +#+                     */
/*                                                   +#+                      */
/*   Created: 2025/06/27 13:59:39 by mstencel      #+#    #+#                 */
/*   Updated: 2025/07/07 13:50:42 by mstencel      ########   odam.nl         */
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
	redirectionCode_(-1),
	redirectionsPath_("") {
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
	redirectionCode_(copy.redirectionCode_),
	redirectionsPath_(copy.redirectionsPath_) {
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
		redirectionCode_ = copy.redirectionCode_;
		redirectionsPath_ = copy.redirectionsPath_;
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

int	Location::getRedirectionCode() const {
	return (redirectionCode_);
}

std::string	Location::getRedirectionsPath() const {
	return (redirectionsPath_);
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

void	Location::setRedirectionCode(const int& newRedirectionCode) {
	redirectionCode_ = newRedirectionCode;
}

void	Location::setRedirectionsPath(const std::string& newRedirectionsPath) {
	redirectionsPath_ = newRedirectionsPath;
}

std::ostream& operator<<(std::ostream& os, const Location& location) {
	os << "Location: \n";
	if (!location.getPath().empty())
		os << "\tPath: " << location.getPath() << "\n";
	if (!location.getRoot().empty())
		os << "\tRoot: " << location.getRoot() << "\n";
	if (!location.getIndex().empty())
		os << "\tIndex: " << location.getIndex() << "\n";
	const std::vector<std::string>& allowedMethods = location.getAllowedMethods();
	if (!allowedMethods.empty()){
		os << "\tAllowed methods: ";
		for (const std::string& method : allowedMethods) {
			os << method << " ";
		}
		os << "\n";
	}
	os << "\tAutoindex: " << (location.getAutoindex() ? "on" : "off") << "\n";
	if (!location.getUploadPath().empty())
		os << "\tUpload path: " << location.getUploadPath() << "\n";
	if (location.getRedirectionCode() != -1) {
		os << "\tRedirection code: " << location.getRedirectionCode() << "\n";
		os << "\tRedirection path: " << location.getRedirectionsPath() << "\n";
	}
	return os;
}

