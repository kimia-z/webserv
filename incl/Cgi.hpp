#ifndef CGI_HPP
# define CGI_HPP

#include "Webserv42.hpp"

#include <sys/wait.h>


class Cgi {
	//TODO change the bloody spaces into tabs o_O
	public:
	Cgi(const Request& request, const Location& location);
	~Cgi();


	int getStatusCode() const;
	std::string getContentType() const; // e.g. "textt/html"
	std::string getScriptPath() const; // full cgi script path
	
	std::string runCgi(); //starts CGI and returns the body
	void	buildEnv(); // creates the env for the cgi
	void	parseCgiOutput(); // parses cgi's output to get the body and content type

	class CgiException : public std::exception {
		public:
			CgiException(const std::string& message) : message_(message) {}
			const char* what() const throw();
			~CgiException();

		private:
			std::string message_;
	};

	private:
	const Request& request_;
	const Location& location_;
	std::string scriptPath_; //path to the cgi script
	std::map<std::string, std::string> env_; // environment variables for CGI
	std::string output_; // output from the CGI script
	std::string body_; // body of the response
	std::string contentType_; // e.g. "text/html"
	int statusCode_; // e.g. 200, 404, etc.


};

#endif