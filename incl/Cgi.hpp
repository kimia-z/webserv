#ifndef CGI_HPP
# define CGI_HPP

#include "Webserv42.hpp"

#include <sys/wait.h>
#include "Request.hpp"


class Cgi {
	public:
		Cgi(const Request& request, const std::string& scriptPath);
		~Cgi();


		// int 		getStatusCode() const;
		// std::string getContentType() const; // e.g. "textt/html"


		std::string										runCgi(); //starts CGI, returns the body
		std::unordered_map<std::string, std::string>	buildEnv(); // creates the env for the cgi

	class CgiException : public std::exception {
		public:
			CgiException(const std::string& message);
			const char* what() const throw();
			~CgiException();

		private:
			std::string message_;
	};

	private:
		const Request& request_;
		std::string scriptPath_; //path to the cgi script
		// std::map<std::string, std::string> env_; // environment variables for CGI
		// std::string output_; // output from the CGI script
		// std::string body_; // body of the response
		// std::string contentType_; // e.g. "text/html"
		// int statusCode_; // e.g. 200, 404, etc.


};

#endif