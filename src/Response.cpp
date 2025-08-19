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
	if (body_.empty() && headers_.find("Content-Length") == headers_.end()) {
		if (statusCode_ >= 100 && statusCode_ < 200) {
			// Do nothing for 1xx responses, as they have no body.
		}
		else if (statusCode_ == 204 || statusCode_ == 304) {
			// 204 :successful but there is no new information to send back to the client.
			// 304 :successful but already have the most recent version of this file in the cache, so just use that instead of downloading it again
			// Do nothing for 204 and 304 responses, as they also have no body.
		}
		else {
			// For all other status codes, if the body is empty, we must specify
			// a Content-Length of 0.
			setHeader("Content-Length", "0");
		}
	}
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

void Response::buildErrorResponse(int statusCode, const std::string& customErrorPageContent){
	setStatusCode(statusCode);
	setHeader("Content-Type", "text/html");
	if (!customErrorPageContent.empty()) {
		setBody(customErrorPageContent);
	} else {
		std::stringstream html_body;
		html_body << "<!DOCTYPE html>\r\n"
				  << "<html>\r\n"
				  << "<head><title>" << statusCode_ << " " << statusMessage_ << "</title></head>\r\n"
				  << "<body>\r\n"
				  << "<h1>" << statusCode_ << " " << statusMessage_ << "</h1>\r\n"
				  << "<p>The requested resource could not be found or processed.</p>\r\n"
				  << "</body>\r\n"
				  << "</html>\r\n";
		setBody(html_body.str());
	}
}
void Response::buildRedirectResponse(int statusCode, const std::string& locationUrl){
	setStatusCode(statusCode);
	setHeader("Location", locationUrl);
	setBody("");
}
void Response::buildStaticFileResponse(int statusCode, const std::string& fileContent, const std::string& contentType){
	setStatusCode(statusCode);
	setBody(fileContent);
	setHeader("Content-Type", contentType);
}
void Response::buildSimpleTextResponse(int statusCode, const std::string& bodyText, const std::string& contentType){
	setStatusCode(statusCode);
	setBody(bodyText);
	setHeader("Content-Type", contentType);
}
void Response::buildFromAction(const ActionParameters& action, const std::string& content, int actionStatusCode){

	if (action.errorCode != 0) {
		buildErrorResponse(actionStatusCode, content);
	}
	else if (action.isRedirect) {
		buildRedirectResponse(actionStatusCode, content);
	}
	else if (action.isStaticFile) {
		if (action.isAutoindex) {
			buildStaticFileResponse(actionStatusCode, content, "text/html"); 
		}
		else {
			buildStaticFileResponse(actionStatusCode, content, getMimeType(action.filePath));
		}
	}
	else {
		if (content.empty()){
			buildSimpleTextResponse(200, "<h1>Default Success Page</h1><p>Request processed.</p>", "text/html");
		} else {
			buildSimpleTextResponse(200, content, "text/html");
		}
	}
}

// Helper
std::string Response::getMimeType(const std::string& filePath) const {
	size_t dotPos = filePath.rfind('.');
	if (dotPos == std::string::npos) return "application/octet-stream";

	std::string ext = filePath.substr(dotPos);
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower); // Case-insensitive

	if (ext == ".html" || ext == ".htm") return "text/html";
	else if (ext == ".css") return "text/css";
	else if (ext == ".js") return "application/javascript";
	else if (ext == ".json") return "application/json";
	else if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
	else if (ext == ".png") return "image/png";
	else if (ext == ".gif") return "image/gif";
	else if (ext == ".ico") return "image/x-icon";
	else if (ext == ".txt") return "text/plain";
	else if (ext == ".pdf") return "application/pdf";
	else if (ext == ".xml") return "application/xml";
	else if (ext == ".mp3") return "audio/mpeg";
	else if (ext == ".mp4") return "video/mp4";
	return "application/octet-stream";
}
