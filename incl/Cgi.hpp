#ifndef CGI_HPP
# define CGI_HPP

#include "Webserv42.hpp"

#include <sys/wait.h>
#include <functional>
#include "Request.hpp"


class Cgi {

	public:
		Cgi(const Request& request, const std::string& scriptPath);
		~Cgi();

		std::string										runCgi(); //starts CGI, returns the body
		void											startCgi(std::function<void(int, uint32_t)> addtoEpoll); // starts the CGI process, creates pipes
		std::unordered_map<std::string, std::string>	buildEnv(); // creates the env for the cgi

		void											setScriptPath(const std::string& scriptPath);
		void											setCgiStatusCode(int statusCode);
	
		int												getCgiStatusCode() const;

		class CgiException : public std::exception {
		public:
			CgiException(const std::string& message);
			const char* what() const throw();
			~CgiException();

		private:
			std::string message_;
	};

	private:
		const Request&	request_;
		std::string		scriptPath_; //path to the cgi script
		int				cgiStatusCode_; // status code of the cgi script
		int				pipeIn_[2]; // pipe for input to CGI
		int				pipeOut_[2]; // pipe for output from CGI
};

#endif