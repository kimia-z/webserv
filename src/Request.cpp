#include "../incl/Request.hpp"

// Default constructor - important for non-blocking
Request::Request() : _isHeadersComplete(false), _contentLength(-1),
					_isChunked(false), _bodyStartPos(0), _isRequestComplete(false)
{
	// Initialize parsed data members to empty/default values
	_method.clear();
	_path.clear();
	_queryParams.clear();
	_version.clear();
	_headers.clear();
	_body.clear();
	//_codeStatus = 200; // Default status, update on error
}

// You might keep this for testing, but the main flow will use appendRawData
// Request::Request(const std::string &rawRequest) {
//     _rawBuffer = rawRequest; // Store initially
//     processRequestData(); // Attempt to process immediately
// }

void Request::appendRawData(const char* data, size_t len) {
	if (data && len > 0) {
		_rawBuffer.append(data, len);
	}
}

// Central function to check completion and incrementally parse
bool Request::processRequestData() {
	if (_isRequestComplete) { // Already processed a complete request, need to trim first
		return true; // Or false if you want to explicitly wait for trim before next parse
	}

	// Step 1: Check for complete headers if not already done
	if (!_isHeadersComplete) {
		size_t header_end_pos = findCRLFCRLF(_rawBuffer);
		if (header_end_pos == std::string::npos) {
			// Headers are not yet complete, need more data
			return false;
		}
		_bodyStartPos = header_end_pos + 4; // Position after "\r\n\r\n"
		_isHeadersComplete = true; // Mark headers as parsed

		// Now parse the headers since we have the full header block
		try {
			parseStartLineAndHeaders();
		} catch (const HttpException& e) {
			// Handle parsing errors during header stage
			std::cerr << "Header parsing error: " << e.what() << std::endl;
			// You might set an error status code here and mark request complete with error
			_isRequestComplete = true; // Mark as complete with error state
			// _codeStatus = e.getStatusCode();
			return true; // Return true to signal that this (erroneous) request is "processed"
		}
	}

	// Step 2: If headers are complete, check/parse body
	if (_isHeadersComplete) {
		// Check for Content-Length or Transfer-Encoding to determine body completeness
		auto clIt = _headers.find("Content-Length");
		auto teIt = _headers.find("Transfer-Encoding");

		if (teIt != _headers.end() && teIt->second == "chunked") {
			// Check if the final "0\r\n\r\n" is present in the raw buffer
			if (_rawBuffer.length() >= _bodyStartPos + 5) { // Min for "0\r\n\r\n"
				if (_rawBuffer.find("0\r\n\r\n", _bodyStartPos) != std::string::npos) {
					_isRequestComplete = true;
					parseBodyContent(); // Parse the chunked body
				}
			}
		} else if (clIt != _headers.end()) {
			try {
				_contentLength = std::atol(clIt->second.c_str());
				if (_contentLength < 0) { // Invalid Content-Length
					throw HttpException(400, "Bad Request: Negative Content-Length");
				}
				if (static_cast<long>(_rawBuffer.length() - _bodyStartPos) >= _contentLength) {
					_isRequestComplete = true;
					parseBodyContent(); // Parse the body
				}
			} catch (const HttpException& e) {
				std::cerr << "Content-Length error: " << e.what() << std::endl;
				_isRequestComplete = true; // Mark as complete with error
				// _codeStatus = e.getStatusCode();
				return true;
			} catch (const std::exception& e) { // For std::atol conversion errors
				std::cerr << "Invalid Content-Length value: " << e.what() << std::endl;
				_isRequestComplete = true; // Mark as complete with error
				// _codeStatus = 400;
				return true;
			}
		} else if (_method == "POST") {
			// If POST/PUT without CL or TE, it's generally an error (HTTP/1.1 requires one)
			// Unless it's a specific type of POST that's allowed without body.
			// For this project, assume it needs one.
			throw HttpException(400, "Bad Request: Missing Content-Length or Transfer-Encoding for POST/PUT");
		} else {
			// GET, DELETE, or other methods without a body
			_isRequestComplete = true; // Request is complete once headers are done
		}
	}
	return _isRequestComplete;
}

// Parses the request line and headers from _rawBuffer (called once headers are complete)
void Request::parseStartLineAndHeaders() {
	std::istringstream stream(_rawBuffer.substr(0, _bodyStartPos)); // Only pass header portion
	std::string line;

	// Step 1: Parse start-line: METHOD PATH VERSION
	if (!std::getline(stream, line)) { // Read first line
		throw HttpException(400, "Bad Request: Empty request line");
	}
	if (line.back() == '\r') line.pop_back(); // Remove trailing \r
	if (!parseStartLine(line)){ // Use your existing parseStartLine
		throw HttpException(400, "Bad Request: Invalid start line");
	}

	// Step 2: Parse headers
	while(std::getline(stream, line)) {
		if (line == "\r" || line.empty()) break; // End of headers
		if (line.back() == '\r') line.pop_back(); // Remove trailing \r

		if (!parseHeader(line)){ // Use your existing parseHeader
			throw HttpException(400, "Bad Request: Invalid Header");
		}
	}
	// Header validation: Mandatory Host name in header
	if (_headers.find("Host") == _headers.end()){
		throw HttpException(400, "Bad Request: Missing mandatory 'Host' header");
	}
}

// Parses the body from _rawBuffer (called once _requestComplete is true)
void Request::parseBodyContent() {
	if (_bodyStartPos >= _rawBuffer.length()) {
		_body.clear(); // No body found
		return;
	}

	std::string rawBodyPart = _rawBuffer.substr(_bodyStartPos);

	if (_isChunked) {
		// Your existing parseChunkedBody needs to be adapted or called with rawBodyPart.
		// Note: parseChunkedBody currently takes rawRequest, it needs to take the body part only
		// And potentially handle partial chunks if you want full incremental parsing
		_body = decodeChunkedBody(rawBodyPart); // You'll need to implement/adapt this.
		if (_body.empty() && rawBodyPart.length() > 0 && rawBodyPart.find("0\r\n\r\n") == std::string::npos) {
			// If decodeChunkedBody returns empty but the chunked data is not empty AND not complete
			// this means it's an incomplete chunked body. This scenario should ideally
			// be handled by checkCompletion, but it's a double check.
			throw HttpException(400, "Incomplete chunked body.");
		}
	} else if (_contentLength != -1) {
		// Your existing parseBody would be integrated here.
		// It just needs to take the correct substring and handle length checks.
		if (static_cast<long>(rawBodyPart.length()) < _contentLength) {
			// This shouldn't happen if _requestComplete is true and Content-Length valid.
			throw HttpException(400, "Body shorter than Content-Length after completion check.");
		}
		_body = rawBodyPart.substr(0, _contentLength);
	} else {
		// No body expected or parsed.
		_body.clear();
	}
}

// Original parsing helpers (adapt where needed, especially `parseBody` will change)
// ... parseStartLine, parseQueryParams, parseHeader, isValidMethod, isValidPath, isValidVersion, isValidKey, isValidValue ...
// Make sure they are 'const' if they don't modify member variables:
// bool Request::isValidMethod(const std::string &method) const { ... }

// Adapting parseChunkedBody: It should take the rawBodyPart string
// You'll need to implement or integrate `decodeChunkedBody` function here
// or modify `parseChunkedBody` to return the decoded body.
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


		if (chunk_size == 0) {
			// End of chunked stream (0\r\n\r\n)
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


// New method to clear a parsed request from the buffer
void Request::clearParsedRequest() {
	if (_isRequestComplete) { // Only clear if a complete request was found and processed
		size_t bytesToErase = 0;
		if (_isHeadersComplete) { // Headers are always needed to define the request boundary
			bytesToErase = _bodyStartPos; // Up to end of headers (including "\r\n\r\n")

			if (_isChunked) {
				// For chunked, find the exact end of the last chunk marker "0\r\n\r\n"
				size_t chunked_end = _rawBuffer.find("0\r\n\r\n", _bodyStartPos);
				if (chunked_end != std::string::npos) {
					bytesToErase = chunked_end + 5; // "0\r\n\r\n" is 5 chars
				} else {
					// This indicates an error, as _requestComplete should only be true if 0\r\n\r\n was found.
					std::cerr << "Warning: Inconsistent state - chunked request marked complete but 0\\r\\n\\r\\n not found during trim." << std::endl;
					// Attempt to find any double CRLF after body start as a fallback, or just return.
					bytesToErase = _rawBuffer.length(); // Fallback: clear everything
				}
			} else if (_contentLength != -1) {
				bytesToErase += _contentLength;
			}
			// If no body or non-chunked, _bodyStartPos is the end.
		}

		// Ensure bytesToErase does not exceed _rawBuffer length
		if (bytesToErase > _rawBuffer.length()) {
			bytesToErase = _rawBuffer.length(); // Defensive check
		}

		_rawBuffer.erase(0, bytesToErase);
	}
	reset(); // Reset internal state for the *next* request
}

// Resets the Request object to its initial state for a new request
void Request::reset() {
	_method.clear();
	_path.clear();
	_queryParams.clear();
	_version.clear();
	_headers.clear();
	_body.clear();
	_isHeadersComplete = false;
	_contentLength = -1;
	_isChunked = false;
	_bodyStartPos = 0;
	_isRequestComplete = false;
	// _codeStatus = 200;
}

bool Request::isRequestComplete() const {return _isRequestComplete;}

const std::string& Request::getRawBuffer() const {return _rawBuffer;}

const std::unordered_map<std::string, std::string> &Request::getQueryParams() const { return _queryParams; }

size_t Request::findCRLFCRLF(const std::string& buffer) const {return buffer.find("\r\n\r\n");}


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