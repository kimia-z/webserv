#ifndef REQUEST_HPP
# define REQUEST_HPP

#include "Webserv42.hpp"
#include <sstream>
#include <iostream>
#include <set>
#include <algorithm>
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
	// int												_codeStatus;

	// non-blocking fixing
	std::string	_rawBuffer; // Accumulates raw data from recv()
	bool		_isHeadersComplete; // True when "\r\n\r\n" is found
	long		_contentLength; // Parsed from Content-Length header
	bool		_isChunked; // True if Transfer-Encoding: chunked
	size_t		_bodyStartPos; // Start index of the body in _rawBuffer
	bool		_isRequestComplete; // True when full request (headers + body) received


	// Parser
	// void parseRequest(const std::string &rawRequest);
	bool parseStartLine(std::string line);
	void parseQueryParams(std::string &queryString);
	bool parseHeader(std::string line);
	// void parseChunkedBody(const std::string &rawRequest);
	// void parseBody(const std::string &rawRequest);

	void parseStartLineAndHeaders(); // Parses if headers are complete
	void parseBodyContent(); // Parses body if complete (or incrementally)

	// Helper to find end of headers
	size_t findCRLFCRLF(const std::string& buffer) const;
	std::string decodeChunkedBody(const std::string& chunked_data);

	// Validation
	bool isValidMethod(const std::string &method) const;
	bool isValidPath(const std::string &path) const;
	bool isValidVersion(const std::string &version) const;
	bool isValidKey(const std::string &key) const;
	bool isValidValue(const std::string &value) const;

public:
	Request();
	// Request(const Request& copy);
	// Request& operator=(const Request& copy);
	// ~Request();
	
	// Appends new raw data received from recv() to the internal buffer
	void appendRawData(const char* data, size_t len);

	// Attempts to incrementally parse the request from _rawBuffer.
	// Returns true if a complete request is now available in _rawBuffer and parsed.
	// This is the core "check and parse one request" method.
	bool processRequestData(); 

	// Removes the parsed request from the internal _rawBuffer.
	// Used when a complete request has been handled and we're looking for the next one.
	void clearParsedRequest(); 

	// Resets the internal state for processing a new request on the same connection.
	void reset();

	//Getters
	const std::string &getMethod() const;
	const std::string &getPath() const;
	const std::string &getVersion() const;
	const std::unordered_map<std::string, std::string> &getHeaders() const;
	const std::string &getBody() const;
	const std::unordered_map<std::string, std::string> &getQueryParams() const;

	// State Getters
	bool isRequestComplete() const;
	const std::string& getRawBuffer() const; // For debugging remaining data
	
	//Print for debugging
	void print() const;
};

#endif
