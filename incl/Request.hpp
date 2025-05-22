#ifndef REQUEST
# define REQUEST

#include "Webserv42.hpp"
#include <sstream>
#include <iostream>
#include <set>
#include <algorithm>

class Request
{
private:
	std::string _method;
	std::string _path;
	std::unordered_map<std::string, std::string> _queryParams;
	std::string _version;
	std::unordered_map<std::string, std::string> _headers;
	std::string _body;


	// Parser
	void parseRequest(const std::string &rawRequest);
	bool parseStartLine(std::string line);
	void parseQueryParams(std::string &queryString);
	bool parseHeader(std::string line);
	void parseChunkedBody(const std::string &rawRequest);
	void parseBody(const std::string &rawRequest);


	// Validation
	bool isValidMethod(const std::string &method);
	bool isValidPath(const std::string &path);
	bool isValidVersion(const std::string &version);
	bool isValidKey(const std::string &key);
	bool isValidValue(const std::string &value);

public:
	Request(const std::string &rawRequest);
	// Request(const Request& copy);
	// Request& operator=(const Request& copy);
	// ~Request();
	
	//Getters
	const std::string &getMethod() const;
	const std::string &getPath() const;
	const std::string &getVersion() const;
	const std::unordered_map<std::string, std::string> &getHeaders() const;
	// const std::string &getBody() const;

	//Print for debugging
	void print() const;
};

#endif
