#ifndef REQUEST_HPP
# define REQUEST_HPP

#include "Webserv42.hpp"
#include <sstream>
#include <iostream>
#include <set>
#include <algorithm>
#include <unordered_map>
#include "HttpException.hpp"


class Request
{
private:
	std::string										_method;
	std::string										_path;
	std::unordered_map<std::string, std::string>	_queryParams;
	std::string										_version;
	std::unordered_map<std::string, std::string>	_headers;
	std::string										_body;

	std::string	_rawBuffer;
	bool		_isHeadersComplete;
	long		_contentLength;
	bool		_isChunked;
	size_t		_bodyStartPos;
	bool		_isRequestComplete;


	// Parser
	bool		parseStartLine(std::string line);
	void		parseQueryParams(std::string &queryString);
	bool		parseHeader(std::string line);
	void		parseStartLineAndHeaders();
	void		parseBodyContent();

	// Helper
	size_t		findCRLFCRLF(const std::string& buffer) const;
	std::string	decodeChunkedBody(const std::string& chunked_data);

	// Validation
	bool		isValidMethod(const std::string &method) const;
	bool		isValidPath(const std::string &path) const;
	bool		isValidVersion(const std::string &version) const;
	bool		isValidKey(const std::string &key) const;
	bool		isValidValue(const std::string &value) const;
	bool		isRequestComplete() const;

public:
	Request();
	// Request(const Request& copy);
	// Request& operator=(const Request& copy);
	~Request();
	
	void		appendRawData(const char* data, size_t len);
	bool		processRequestData(); 
	void		clearParsedRequest(); 
	void		reset();

	//Getters
	const std::string									&getMethod() const;
	const std::string									&getPath() const;
	const std::string									&getVersion() const;
	const std::unordered_map<std::string, std::string>	&getHeaders() const;
	const std::string									&getBody() const;
	const std::unordered_map<std::string, std::string>	&getQueryParams() const;
	const std::string									&getRawBuffer() const;

	//Print for debugging
	void print() const;
};

#endif
