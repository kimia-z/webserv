#ifndef HTTPEXCEPTION_HPP
#define HTTPEXCEPTION_HPP

#include <exception>
#include <string>

class HttpException : public std::exception
{
	int			_code;
	std::string _msg;

public:
	HttpException(int code, const std::string& msg);

	int getCode() const;
	const std::string &getMessage() const;
	
	const char* what() const noexcept override;
};

#endif
