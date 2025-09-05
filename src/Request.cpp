#include "../incl/Request.hpp"

Request::Request() : _isHeadersComplete(false), _contentLength(-1),
					_isChunked(false), _bodyStartPos(0), _isRequestComplete(false), _maxBodySize(0)
{
	_method.clear();
	_path.clear();
	_queryString.clear();
	_queryParams.clear();
	_version.clear();
	_headers.clear();
	_body.clear();
}

Request::~Request(){}

void Request::appendRawData(const char* data, size_t len) {
	if (data && len > 0) {
		_rawBuffer.append(data, len);
	}
}

bool Request::processRequestData() {
	if (_isRequestComplete) {
		return true;
	}
	// Step 1: Check for complete headers if not already done
	if (!_isHeadersComplete) {
		size_t header_end_pos = findCRLFCRLF(_rawBuffer);
		if (header_end_pos == std::string::npos) {
			// Headers are not yet complete, need more data
			// std::cout << "DEBUG: Headers not complete, need more data. Buffer size: " << _rawBuffer.length() << std::endl;
			return false;
		}
		_bodyStartPos = header_end_pos + 4; // Position after "\r\n\r\n"
		_isHeadersComplete = true;
		// std::cout << "DEBUG: Headers complete, parsing start line and headers" << std::endl;
		try {
			parseStartLineAndHeaders();
			// Check body size limit immediately after parsing headers
			auto clIt = _headers.find("Content-Length");
			if (clIt != _headers.end() && _maxBodySize > 0) {
				long contentLength = std::atol(clIt->second.c_str());
				// std::cout << "DEBUG: Checking body size: " << contentLength << " vs limit: " << _maxBodySize << std::endl;
				if (contentLength > _maxBodySize) {
					// std::cout << "DEBUG: Request body size exceeds limit: " << contentLength << " > " << _maxBodySize << std::endl;
					throw HttpException(413, "Payload Too Large");
				}
			}
		} catch (const HttpException& e) {
			std::cerr << "Header parsing error: " << e.what() << std::endl;
			_isRequestComplete = true;
			throw e;
		}
	}
	// Step 2: check/parse body
	if (_isHeadersComplete) {
		// std::cout << "DEBUG: Headers complete, parsing body. Buffer size: " << _rawBuffer.length() 
		//		  << ", Body start pos: " << _bodyStartPos << std::endl;
		auto clIt = _headers.find("Content-Length");
		auto teIt = _headers.find("Transfer-Encoding");
		if (teIt != _headers.end() && teIt->second == "chunked") {
			// Check if the final "0\r\n\r\n" is present in the raw buffer
			if (_rawBuffer.length() >= _bodyStartPos + 5) { // Min for "0\r\n\r\n"
				if (_rawBuffer.find("0\r\n\r\n", _bodyStartPos) != std::string::npos) {
					_isRequestComplete = true;
					parseBodyContent();
				}
			}
		} else if (clIt != _headers.end()) {
			try {
				_contentLength = std::atol(clIt->second.c_str());
				// std::cout << "DEBUG: Content-Length: " << _contentLength << ", Current body size: " 
				//		  << (_rawBuffer.length() - _bodyStartPos) << std::endl;
				if (_contentLength < 0) {
					throw HttpException(400, "Bad Request: Negative Content-Length");
				}
				if (static_cast<long>(_rawBuffer.length() - _bodyStartPos) >= _contentLength) {
					// std::cout << "DEBUG: Body complete, parsing content" << std::endl;
					_isRequestComplete = true;
					parseBodyContent();
				} else {
					// std::cout << "DEBUG: Body not complete, need more data" << std::endl;
				}
			} catch (const HttpException& e) {
				std::cerr << "Content-Length error: " << e.what() << std::endl;
				_isRequestComplete = true;
				throw e;
			}
		} else if (_method == "POST") {
			throw HttpException(400, "Bad Request: Missing Content-Length or Transfer-Encoding for POST/PUT");
		} else {
			// GET, DELETE, ...
			_isRequestComplete = true;
		}
	}
	// for (auto const& header : _headers) {
	// 	std::cout << header.first << std::endl;
	// }
	return _isRequestComplete;
}

void Request::parseStartLineAndHeaders() {
	std::istringstream stream(_rawBuffer.substr(0, _bodyStartPos));
	std::string line;

	// Step 1: Parse start-line: METHOD PATH VERSION
	if (!std::getline(stream, line)) {
		throw HttpException(400, "Bad Request: Empty request line");
	}
	if (line.back() == '\r') line.pop_back(); // Remove \r
	if (!parseStartLine(line)){
		throw HttpException(400, "Bad Request: Invalid start line");
	}
	std::cout << "parsed start line: " << _method << " " << _path << " " << _version << std::endl;
	// Step 2: Parse headers
	while(std::getline(stream, line)) {
		if (line == "\r" || line.empty()) break; // End of headers
		if (line.back() == '\r') line.pop_back(); // Remove \r
		if (!parseHeader(line)){
			throw HttpException(400, "Bad Request: Invalid Header");
		}
	}
	// Header validation: Mandatory Host name in header
	if (_headers.find("Host") == _headers.end()){
		throw HttpException(400, "Bad Request: Missing mandatory 'Host' header");
	}
}

void Request::parseBodyContent() {
	if (_bodyStartPos >= _rawBuffer.length()) {
		_body.clear();
		return;
	}

	std::string rawBodyPart = _rawBuffer.substr(_bodyStartPos);

	if (_isChunked) {
		_body = decodeChunkedBody(rawBodyPart);
		if (_body.empty() && rawBodyPart.length() > 0 && rawBodyPart.find("0\r\n\r\n") == std::string::npos) {
			throw HttpException(400, "Incomplete chunked body.");
		}
	} else if (_contentLength != -1) {
		if (static_cast<long>(rawBodyPart.length()) < _contentLength) {
			throw HttpException(400, "Body shorter than Content-Length after completion check.");
		}
		_body = rawBodyPart.substr(0, _contentLength);
	} else {
		_body.clear();
	}
}

std::string Request::decodeChunkedBody(const std::string& chunked_data) {
	std::string decoded_body;
	size_t pos = 0;
	while (pos < chunked_data.length()) {
		size_t chunk_size_end = chunked_data.find("\r\n", pos);
		if (chunk_size_end == std::string::npos) {
			// Malformed chunked encoding or incomplete data
			throw HttpException(400, "Malformed chunked encoding: missing chunk size CRLF.");
		}
		std::string size_str = chunked_data.substr(pos, chunk_size_end - pos);
		long chunk_size = 0;
		try {
			 chunk_size = std::strtol(size_str.c_str(), NULL, 16); // Hex to long
		} catch (const std::exception& e) {
			 throw HttpException(400, "Malformed chunked encoding: invalid chunk size.");
		}


		if (chunk_size == 0) { // End of chunked stream (0\r\n\r\n)
			// Check for final \r\n
			if (chunk_size_end + 2 + 2 <= chunked_data.length() &&
				chunked_data.substr(chunk_size_end + 2, 2) == "\r\n") {
				pos = chunk_size_end + 4; // Move past "0\r\n\r\n"
				break; // Fully decoded
			} else {
				throw HttpException(400, "Malformed chunked encoding: invalid final chunk.");
			}
		}

		pos = chunk_size_end + 2; // Move past chunk size and first \r\n

		if (pos + chunk_size > chunked_data.length()) { // Check if we have enough data for the chunk
			throw HttpException(400, "Incomplete chunked body: not enough data for chunk.");
		}

		decoded_body.append(chunked_data.substr(pos, chunk_size));
		pos += chunk_size;

		if (pos + 2 > chunked_data.length() || chunked_data.substr(pos, 2) != "\r\n") {
			throw HttpException(400, "Malformed chunked encoding: missing trailing CRLF for chunk.");
		}
		pos += 2; // Move past trailing \r\n
	}
	return decoded_body;
}

void Request::clearParsedRequest() {
	if (_isRequestComplete) {
		size_t bytesToErase = 0;
		if (_isHeadersComplete) {
			bytesToErase = _bodyStartPos;
			if (_isChunked) {
				// For chunked, find the exact end of the last chunk marker "0\r\n\r\n"
				size_t chunked_end = _rawBuffer.find("0\r\n\r\n", _bodyStartPos);
				if (chunked_end != std::string::npos) {
					bytesToErase = chunked_end + 5; // "0\r\n\r\n" is 5 chars
				} else {
					std::cerr << "Warning: Inconsistent state - chunked request marked complete but 0\\r\\n\\r\\n not found during trim." << std::endl;
					bytesToErase = _rawBuffer.length(); // clear everything
				}
			} else if (_contentLength != -1) {
				bytesToErase += _contentLength;
			}
		}
		if (bytesToErase > _rawBuffer.length()) {
			bytesToErase = _rawBuffer.length();
		}
		_rawBuffer.erase(0, bytesToErase);
	}
	reset();
}

void Request::reset() {
	_method.clear();
	_path.clear();
	_queryString.clear();
	_queryParams.clear();
	_version.clear();
	_headers.clear();
	_body.clear();
	_isHeadersComplete = false;
	_contentLength = -1;
	_isChunked = false;
	_bodyStartPos = 0;
	_isRequestComplete = false;
}

void Request::setMaxBodySize(long maxBodySize) {
	_maxBodySize = maxBodySize;
}

void Request::markComplete() {
	_isRequestComplete = true;
}

size_t Request::findCRLFCRLF(const std::string& buffer) const {
	return buffer.find("\r\n\r\n");}

bool Request::parseStartLine(std::string line)
{
	std::istringstream start_line(line);
	std::string fullPath;

	start_line >> _method >> fullPath >> _version;

	if (!isValidMethod(_method) || !isValidVersion(_version)) return false;
	size_t qMarkPos = fullPath.find('?');
	if(qMarkPos != std::string::npos){
		_path = fullPath.substr(0, qMarkPos);
		_queryString = fullPath.substr(qMarkPos + 1);
		parseQueryParams(_queryString);
	} else{
		_path = fullPath;
		_queryString.clear();
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

	// Searches the string for the first character that does not match any of the characters specified in its arguments.
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
	// std::cout << "Parsed header: " << key << ": " << value << std::endl;
	return true;
}

// Validations:
bool Request::isValidMethod(const std::string &method) const
{
	static const std::set<std::string> allowedMethods = {
		"GET", "POST", "DELETE"
	};
	return allowedMethods.count(method);
}
bool Request::isValidPath(const std::string &path) const
{
	if (path.empty()) return false; // It must not be empty
	if (path[0] != '/') return false; // It must start with /
	if (path.find("..") != std::string::npos) return false; // It must not contain path traversal for Security risk
	if (path.find('\0') != std::string::npos) return false; // It should not contain null bytes
	return true;
}
bool Request::isValidVersion(const std::string &version) const
{
	return version == "HTTP/1.1";
}

bool Request::isValidKey(const std::string &key) const
{
	for (char c : key){
		if (!std::isalnum(c) && std::string("!#$%&'*+-.^_`|~").find(c) == std::string::npos) return false;
	}
	return true;
}

bool Request::isValidValue(const std::string &value) const
{
	for (char c : value){
		if ((c < 32 && c != 9) || c == 127) return false;
	}
	return true;
}

bool Request::isRequestComplete() const {return _isRequestComplete;}


// Getters:
const std::string &Request::getMethod() const { return _method; }
const std::string &Request::getPath() const { return _path; }
const std::string &Request::getQueryString() const { return _queryString; }
const std::string &Request::getVersion() const { return _version; }
const std::unordered_map<std::string, std::string> &Request::getHeaders() const { return _headers; }
const std::string &Request::getBody() const { return _body; }
const std::string &Request::getRawBuffer() const {return _rawBuffer;}
const std::unordered_map<std::string, std::string> &Request::getQueryParams() const { return _queryParams; }

// Print
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

bool Request::isBodySizeValid(long maxBodySize) const {
	if (maxBodySize == -1) {
		return true; // No limit set
	}
	
	auto clIt = _headers.find("Content-Length");
	if (clIt != _headers.end()) {
		try {
			long contentLength = std::atol(clIt->second.c_str());
			return contentLength <= maxBodySize;
		} catch (...) {
			return false; // Invalid Content-Length
		}
	}
	
	// No Content-Length header, check if it's a method that should have one
	return (_method != "POST" && _method != "PUT");
}