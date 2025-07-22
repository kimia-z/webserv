#ifndef RESPONSE_HPP
# define RESPONSE_HPP

#include "Webserv42.hpp"

class Response
{
private:
	int												statusCode_;
	std::string										statusMessage_;
	std::unordered_map<std::string, std::string>	headers_;
	std::string										body_;
	std::string										protocolVersion_;

	std::string getDefaultStatusMessage(int code) const;
	void addMandatoryHeaders();
public:
	Response();
	~Response();

	std::string toString() const;

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
};

#endif