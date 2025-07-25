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
		// Status codes 204 (No Content), 1xx (Informational), and 304 (Not Modified)
		// should not have a body, so Content-Length should be omitted or 0.
		// For other status codes with an empty body, Content-Length should be 0.
		if (statusCode_ != 204 && (statusCode_ < 100 || statusCode_ >= 200)) {
			// Note: 1xx and 304 have no body so no Content-Length is needed
			// For other empty bodies, a Content-Length of 0 is appropriate.
			setHeader("Content-Length", "0");
		}
	}
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
void Response::buildStaticFileResponse(const std::string& fileContent, const std::string& mimeType, int statusCode){
	setStatusCode(statusCode);
	setBody(fileContent);
	setHeader("Content-Type", mimeType);
}
void Response::buildSimpleTextResponse(int statusCode, const std::string& bodyText, const std::string& contentType = "text/html"){
	setStatusCode(statusCode);
	setBody(bodyText);
	setHeader("Content-Type", contentType);
}
void Response::buildFromAction(const ActionParameters& action, const std::string& content = "", int actionStatusCode = 0){
	
	// Reset internal state for a fresh response
	setStatusCode(200); // Default OK
	headers_.clear();
	body_.clear();

	// Priority 1: Router-determined errors
	if (action.errorCode != 0) {
		buildErrorResponse(action.errorCode, content);
	}
	// Priority 2: Router-determined Redirects
	else if (action.isRedirect) {
		buildRedirectResponse(action.redirectCode, action.redirectUrl);
	}
	// Priority 3: Static File Serving (GET/HEAD requests)
	// This is the primary case for 'building a usual response'
	else if (action.isStaticFile) {
		// 'content' here is the file's body or the directory listing HTML, provided by Server42.
		// 'action.filePath' is used to get the MIME type.
		buildStaticFileResponse(content, getMimeType(action.filePath), 200); // Status 200 OK
	}
	else if (actionStatusCode != 0) {
		// If it's a 204 (No Content, typically from DELETE success), the 'content' string will be empty.
		if (actionStatusCode == 204) {
			setStatusCode(204); // Explicitly set No Content, no body
			setBody(""); // Ensure no body
		} else {
			buildSimpleTextResponse(actionStatusCode, content, "text/html");
		}
	}
	else {
		buildSimpleTextResponse(200, "<h1>Default Success Page</h1><p>Request processed.</p>", "text/html");
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
	// Add more as needed
	
	return "application/octet-stream";
}

/*
Scenario 1: Error Response (if (action.errorCode != 0))

	Action: Call response_object.setStatusCode(action.errorCode);

	Action: If action.errorPagePath is provided by the router, read that file's content and set it as the body: response_object.setBody(readFileContent(action.errorPagePath)); (or use a default HTML error page if no custom path is given).

	Action: Set Content-Type: text/html.

	Router's Contribution: The Router identified which error occurred and which custom error page to use (if any).

Scenario 2: Redirect Response (else if (action.isRedirect))

	Action: Call response_object.setStatusCode(action.redirectCode);

	Action: Add the Location header: response_object.addHeader("Location", action.redirectUrl);

	Action: Set an empty or minimal body.

	Router's Contribution: The Router flagged it as a redirect and provided the redirect URL and code.

Scenario 3: Static File Response (else if (action.isStaticFile))

	Action: If action.isAutoindex is true, generate an HTML directory listing and set it as the body.

	Action: Else, read the file content from action.filePath using readFileContent() and set it as the body.

	Action: Determine the Content-Type based on the file extension (getMimeType(action.filePath)) and add it as a header.

	Action: Set Content-Length (this is automatically handled if setBody updates it).

	Router's Contribution: The Router indicated it's a static file request, provided the exact filePath, and whether autoindex should be used.
*/