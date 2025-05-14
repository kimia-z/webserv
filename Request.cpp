#include "incl/Request.hpp"

Request::Request(const std::string &rawRequest)
{
	parseRequest(rawRequest);
}


void Request::parseRequest(const std::string &rawRequest)
{
	// find start line and call its function
	// loop through headers and call its function for each line
	// check if body exist
	// validation

	std::istringstream stream(rawRequest);
	std::string line;

	// Step 1: Skip empty lines before the start-line
	while (std::getline(stream, line))
	{
		if (line == "\r" || line.empty()) continue;
		break; // Found the start-line
	}
	// Step 2: Parse start-line: METHOD PATH VERSION
	if (!parseStartLine(line)){
		std::cerr << RED << "Parsing the startline failed: " << strerror(errno) << RESET << std::endl;
		// TODO: return false/exit???
	}
	// Step 3: Parse headers
	while(std::getline(stream, line))
	{
		if (line == "\r" || line.empty()) break;
		if (!parseHeader(line)){
			std::cerr << RED << "Invalid header line: " << strerror(errno) << RESET << std::endl;
			continue; // or TODO: return false/exit???
		}
	}
	// Step 4: (Optional) Read message body??
}

//TODO: what should it beheaves when failed in parsing?
bool Request::parseStartLine(std::string line)
{
	std::istringstream start_line(line);

	if (!(start_line >> _method >> _path >> _version)){
		return false;
	}
	return true;
}

bool Request::parseHeader(std::string line)
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


void Request::print() const
{
	std::cout << "Method: " << getMethod() << std::endl;
	std::cout << "Path: " << getPath() << std::endl;
	std::cout << "Version: " << getVersion() << std::endl;
	std::unordered_map<std::string, std::string> requestMap = getHeaders();
	for (auto i : requestMap){
		std::cout << i.first << ": " << i.second << std::endl;
	}
}