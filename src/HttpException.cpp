#include "../incl/HttpException.hpp"

HttpException::HttpException(int code, const std::string& msg)
	: _code(code), _msg(msg) {}

int HttpException::getCode() const {
	return _code;
}

const std::string& HttpException::getMessage() const {
	return _msg;
}

const char *HttpException::what() const noexcept {
	return _msg.c_str();
}
