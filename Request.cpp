#include "incl/Request.hpp"

Request::Request(const std::string &rawRequest)
{
	parseRequest(rawRequest);
}

// Request(const Request& copy);
// Request& operator=(const Request& copy);
// ~Request();


// Parsings:
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

	if (!(start_line >> _method >> _path >> _version)) return false;
	if (!isValidMethod(_method) || !isValidPath(_path) || !isValidVersion(_version)) return false;
	return true;
}

bool Request::parseHeader(std::string line)
{
	// Reject header line starting with whitespace
	if (std::isspace(line[0])) return false;

	std::string copyLine = line;
	if (!copyLine.empty() && copyLine.back() == '\r') {
		copyLine.pop_back();
	}
	size_t colonPos = copyLine.find(':');
	if (colonPos == std::string::npos) return false; 

	std::string key = copyLine.substr(0, colonPos);
	std::string value = copyLine.substr(colonPos + 1);

	// Trim leading and trailing whitespace from value
	// *Searches the string for the first character that
	// 		does not match any of the characters specified in its arguments.*
	size_t startPos = value.find_first_not_of(" \t");
	size_t endPos = value.find_last_not_of(" \t");
	if (startPos != std::string::npos && endPos != std::string::npos){
		value = value.substr(startPos, endPos - startPos + 1);
	}
	else
		value = ""; // value is only spaces
	_headers[key] = value;
	return true;
}


// Validations:
bool Request::isValidMethod(std::string &method){
	static const std::set<std::string> allowedMethods = {
		"GET", "POST", "DELETE"
	};
	return allowedMethods.count(method);
}
bool Request::isValidPath(std::string &path){
	if (path.empty()) return false; // It must not be empty
	if (path[0] != '/') return false; // It must start with /
	if (path.find("..") != std::string::npos) return false; // It must not contain path traversal for Security risk
	if (path.find('\0') != std::string::npos) return false; // It should not contain null bytes
	return true;
}
bool Request::isValidVersion(std::string &version){
	return version == "HTTP/1.1";
}


// Getters:
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