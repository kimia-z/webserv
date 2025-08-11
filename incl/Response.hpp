#ifndef RESPONSE_HPP
# define RESPONSE_HPP

#include "Webserv42.hpp"
#include <iostream>
#include <unordered_map> // for std::unordered_map

struct ActionParameters;

class Response
{
private:
	int												statusCode_;
	std::string										statusMessage_;
	std::unordered_map<std::string, std::string>	headers_;
	std::string										body_;
	std::string										protocolVersion_;

	std::string	getDefaultStatusMessage(int code) const;
	void		addMandatoryHeaders();
	void		buildErrorResponse(int statusCode, const std::string& customErrorPageContent);
	void		buildRedirectResponse(int statusCode, const std::string& locationUrl);
	void		buildStaticFileResponse(const std::string& fileContent, const std::string& mimeType, int statusCode);
	void		buildSimpleTextResponse(int statusCode, const std::string& bodyText, const std::string& contentType = "text/html");
public:
	Response();
	~Response();

	std::string	toString() const;
	void		buildFromAction(const ActionParameters& action, const std::string& content = "", int actionStatusCode = 0);

	// Setters
	void setStatusCode(int code);
	void setHeader(const std::string &key, const std::string &value);
	void setBody(const std::string &body);
	void setProtocolVersion(const std::string &version);

	// Getters
	int getStatusCode() const;
	const std::string &getBody() const;
	const std::string &getProtocolVersion() const;
	const std::unordered_map<std::string, std::string> &getHeaders();

	// Helper
	std::string	getMimeType(const std::string& filePath) const;
};

#endif