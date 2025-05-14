#include "Request.hpp"

Request::Request(const std::string &rawRequest)
{
	parseRequest(rawRequest);
}


void Request::parseRequest(const std::string &rawRequest)
{
	// find start line and call its function
	// loop through headers and call its function for each line
	// check if body exist
}
void Request::parseStartLine()
{

}
void Request::parseHeader()
{

}

// Request(const Request& copy);
// Request& operator=(const Request& copy);
// ~Request();

const std::string &Request::getMethod() const
{
	return _method;
}
const std::string &Request::getPath() const
{
	return _path;
}
const std::string &Request::getVersion() const
{
	return _version;
}
const std::unordered_map<std::string, std::string> &Request::getHeaders() const
{
	return _headers;
}