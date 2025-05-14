#ifndef REQUEST
# define REQUEST

#include "Webserv42.hpp"

class Request
{
private:
	std::string _method;
	std::string _path;
	std::string _version;
	std::unordered_map<std::string, std::string> _headers;
	// std::string _body;

	void parseRequest(const std::string &rawRequest);
	void parseStartLine();
	void parseHeader();
	// void parseBody();

public:
	Request(const std::string &rawRequest);
	Request(const Request& copy);
	Request& operator=(const Request& copy);
	~Request();
	
	//Getters
	const std::string &getMethod() const;
	const std::string &getPath() const;
	const std::string &getVersion() const;
	const std::unordered_map<std::string, std::string> &getHeaders() const;
	// const std::string &getBody() const;
};

#endif
