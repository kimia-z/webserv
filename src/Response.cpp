#include "../incl/Response.hpp"


Response::Response() : statusCode_(200), statusMessage_("OK"), protocolVersion_("HTTP/1.1") {}
Response::~Response(){};


std::string Response::getDefaultStatusMessage(int code) const{
		switch (code) {
		case 100: return "Continue";
		case 200: return "OK";
		case 201: return "Created";
		case 204: return "No Content";
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 409: return "Conflict";
		case 413: return "Payload Too Large";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 504: return "Gateway Timeout";
		default: return "Unknown Status";
	}
}

void Response::addMandatoryHeaders(){

	setHeader("Server", "webserv/1.0");

	char buffer[100];
	time_t now = time(0);
	struct tm *gmt = gmtime(&now);
	strftime(buffer, sizeof(buffer),"%a, %d %b %Y %H:%M:%S GMT", gmt);
	setHeader("Date", buffer);

	// TODO:content-lengh?
}

std::string Response::toString() const {
	std::stringstream s;

	// Status Line: HTTP/1.1 200 OK
	s << protocolVersion_ << " " << statusCode_ << " " << statusMessage_ << "\r\n";

	// Headers
	Response tempResponse = *this;
	tempResponse.addMandatoryHeaders();

	for (const auto& pair : tempResponse.headers_) {
		s << pair.first << ": " << pair.second << "\r\n";
	}
	// End of Headers
	s << "\r\n";

	// Body
	s << body_;

	return s.str();
}

// Setters
void Response::setStatusCode(int code){
	statusCode_ = code;
	statusMessage_ = getDefaultStatusMessage(code);
}

void Response::setBody(const std::string &body){
	body_ = body;
	setHeader("Content-Length", std::to_string(body_.length()));
}
void Response::setProtocolVersion(const std::string &version){
	protocolVersion_ = version;
}
void Response::setHeader(const std::string &key, const std::string &value){
	headers_[key] = value;
}


// Getters
int Response::getStatusCode() const{
	return statusCode_;
}
const std::string &Response::getBody() const{
	return body_;
}
const std::string &Response::getProtocolVersion() const{
	return protocolVersion_;
}
const std::unordered_map<std::string, std::string> &Response::getHeaders(){
	return headers_;
}
