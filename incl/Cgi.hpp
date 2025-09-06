#ifndef CGI_HPP
# define CGI_HPP

#include "Webserv42.hpp"

#include <sys/wait.h>
#include <functional>
#include "Request.hpp"


class Cgi {

	public:
		Cgi(const Request& request, const std::string& scriptPath, const std::string& serverName = "", int serverPort = 0);
		~Cgi();

		std::string										runCgi(); //starts CGI, returns the body (blocking - for compatibility)
		void											startCgi(std::function<void(int, uint32_t)> addtoEpoll); // starts the CGI process, creates pipes
		std::unordered_map<std::string, std::string>	buildEnv(); // creates the env for the cgi
		bool											isCgiComplete() const;
		std::string										getCgiOutput() const;
		
		// Non-blocking CGI methods
		bool											writeToCgiInput(); // Non-blocking write to CGI input
		bool											readFromCgiOutput(); // Non-blocking read from CGI output
		bool											checkCgiProcess(); // Check if CGI process has finished
		bool											isInputComplete() const;
		bool											isOutputComplete() const;
		int												getInputPipe() const;
		int												getOutputPipe() const;
		void											markOutputComplete();
		void											forceComplete();
		time_t											getStartTime() const;
		void											setUploadDir(const std::string& uploadDir);
		void											setMaxFileSize(size_t maxSize);

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
		pid_t			cgiPid_; // PID of the CGI process
		bool			cgiComplete_; // whether CGI has finished
		std::string		cgiOutput_; // accumulated output from CGI
		std::string		uploadDir_; // upload directory for CGI
		size_t			maxFileSize_; // maximum file size for uploads
		bool			inputWritten_; // whether input has been written to CGI
		bool			outputRead_; // whether output has been read from CGI
		time_t			startTime_; // when CGI process started
		std::string		serverName_; // server name from config
		int				serverPort_; // server port from config

		// Helper methods for CGI process management
		void			createPipes(); // Creates pipes for communication with CGI process
		void			forkCgiProcess(); // Forks the process and executes the CGI script
		void			setupCgiPipes(std::function<void(int, uint32_t)> addtoEpoll); // Sets up epoll for non-blocking I/O
};

#endif