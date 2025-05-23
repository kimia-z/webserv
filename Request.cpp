#include "incl/Request.hpp"

Request::Request(const std::string &rawRequest)
{
	try {
		parseRequest(rawRequest);
	} catch (const HttpException &e) {
		std::cerr << "Request parsing failed: " << e.what() << std::endl;
		throw; // Re-throw to be handled by caller
	}
}

// Request(const Request& copy);
// Request& operator=(const Request& copy);
// ~Request();


// Parsings:
void Request::parseRequest(const std::string &rawRequest)
{
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
		throw HttpException(400, "Bad Request: Invalid start line");
		// std::cerr << RED << "Parsing the startline failed: " << strerror(errno) << RESET << std::endl;
	}
	// Step 3: Parse headers
	while(std::getline(stream, line))
	{
		if (line == "\r" || line.empty()) break;
		if (!parseHeader(line)){
			throw HttpException(400, "Bad Request: Invalid Header");
			// std::cerr << RED << "Invalid header line: " << strerror(errno) << RESET << std::endl;
		}
	}
	// Header validation: Mandatory Host name in header
	if (_headers.find("Host") == _headers.end()){
		throw HttpException(400, "Bad Request: Missing mandatory 'Host' header");
		//std::cerr << "The request required Host header" << std::endl;
	}

	// Step 4: Parse body
	parseBody(rawRequest);
}

//TODO: what should it beheaves when failed in parsing?
bool Request::parseStartLine(std::string line)
{
	std::istringstream start_line(line);
	std::string fullPath;

	start_line >> _method >> fullPath >> _version;

	if (!isValidMethod(_method) || !isValidVersion(_version)) return false;
	size_t qMarkPos = fullPath.find('?');
	if(qMarkPos != std::string::npos){
		_path = fullPath.substr(0, qMarkPos);
		std::string queryString = fullPath.substr(qMarkPos + 1);
		parseQueryParams(queryString);
	} else{
		_path = fullPath;
	}
	if(!isValidPath(_path)) return false;
	return true;
}

void Request::parseQueryParams(std::string &queryString)
{
	std::istringstream query(queryString);
	std::string token;

	while (std::getline(query, token, '&')) {
		size_t eqPos = token.find('=');
		std::string key = token.substr(0, eqPos);
		std::string value = (eqPos != std::string::npos) ? token.substr(eqPos + 1) : ""; // handle case with key but no value
		_queryParams[key] = value;
	}
}

bool Request::parseHeader(std::string line)
{
	// Reject header line starting with whitespace
	if (line.empty() || std::isspace(line[0])) return false;

	if (line.back() == '\r') line.pop_back();
	size_t colonPos = line.find(':');
	if (colonPos == std::string::npos) return false;

	std::string key = line.substr(0, colonPos);
	std::string value = line.substr(colonPos + 1);
	// Trim leading and trailing whitespace from value
	// *Searches the string for the first character that
	// 	does not match any of the characters specified in its arguments.*
	size_t start = value.find_first_not_of(" \t");
	size_t end = value.find_last_not_of(" \t");
	value = (start != std::string::npos && end != std::string::npos) ? value.substr(start, end - start + 1) : "";// value is only spaces

	if (!isValidKey(key) || !isValidValue(value)) return false;
	// Handle multiple value for a header
	if (_headers.count(key)) {
		_headers[key] += ", " + value;
	} else {
		_headers[key] = value;
	}
	return true;
}

void Request::parseBody(const std::string &rawRequest)
{
	size_t headerEnd = rawRequest.find("\r\n\r\n");
	if (headerEnd == std::string::npos) return;

	std::string body = rawRequest.substr(headerEnd + 4);
	auto isTe = _headers.find("Transfer-Encoding");
	auto clIt = _headers.find("Content-Length");

	if (isTe != _headers.end() && isTe->second == "chunked") {
		parseChunkedBody(body);
	} else if (clIt != _headers.end()) {
		try {
			int contentLength = std::stoi(clIt->second);
			if (contentLength < 0)
				throw HttpException(400, "Negative Content-Length");
			if ((int)body.size() < contentLength)
				throw HttpException(400, "Body shorter than Content-Length");
			_body = body.substr(0, contentLength);
		} catch (const std::exception &) {
			throw HttpException(400, "Invalid Content-Length value");
		}
	} else if (_method == "POST" || _method == "PUT") {
		throw HttpException(400, "Missing Content-Length or Transfer-Encoding for POST/PUT");
	}
}


void Request::parseChunkedBody(const std::string &rawRequest)
{
	std::istringstream stream(rawRequest);
	std::string line;
	_body.clear();
	while(std::getline(stream, line)){
		if (!line.empty() && line.back() == '\r') line.pop_back();
		size_t chunkedSize = std::stoul(line, nullptr, 16);
		if (chunkedSize == 0) break;
		char *buffer = new char[chunkedSize];
		stream.read(buffer, chunkedSize);
		_body.append(buffer, chunkedSize);
		delete [] buffer;
		stream.ignore(2);
	}

	// Parse tailer headers????
}

// Validations:
bool Request::isValidMethod(const std::string &method){
	static const std::set<std::string> allowedMethods = {
		"GET", "POST", "DELETE"
	};
	return allowedMethods.count(method);
}
bool Request::isValidPath(const std::string &path){
	if (path.empty()) return false; // It must not be empty
	if (path[0] != '/') return false; // It must start with /
	if (path.find("..") != std::string::npos) return false; // It must not contain path traversal for Security risk
	if (path.find('\0') != std::string::npos) return false; // It should not contain null bytes
	return true;
}
bool Request::isValidVersion(const std::string &version){
	return version == "HTTP/1.1";
}

bool Request::isValidKey(const std::string &key)
{
	for (char c : key){
		if (!std::isalnum(c) && std::string("!#$%&'*+-.^_`|~").find(c) == std::string::npos) return false;
	}
	return true;
}

bool Request::isValidValue(const std::string &value)
{
	for (char c : value){
		if ((c < 32 && c != 9) || c == 127) return false;
	}
	return true;
}

// Getters:
const std::string &Request::getMethod() const { return _method; }
const std::string &Request::getPath() const { return _path; }
const std::string &Request::getVersion() const { return _version; }
const std::unordered_map<std::string, std::string> &Request::getHeaders() const { return _headers; }
const std::string &Request::getBody() const { return _body; }


void Request::print() const
{
	std::cout << "Method: " << getMethod() << std::endl;
	std::cout << "Path: " << getPath() << std::endl;
	std::cout << "Version: " << getVersion() << std::endl;
	std::unordered_map<std::string, std::string> requestMap = getHeaders();
	for (auto i : requestMap){
		std::cout << i.first << ": " << i.second << std::endl;
	}
	std::cout << "Body: " << getBody() << std::endl;
}