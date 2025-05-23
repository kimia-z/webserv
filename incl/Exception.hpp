#ifndef EXCEPTION_HPP
#define EXCEPTION_HPP

#include <exception>
#include <string>

class HttpException : public std::exception
{
	int _code;
	std::string _msg;

public:
	HttpException(int code, const std::string& msg)
		: _code(code), _msg(msg) {}

	int code() const { return _code; }

	const char* what() const noexcept override {
		return _msg.c_str();
	}
};

#endif
