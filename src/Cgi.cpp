#include "Cgi.hpp"
#include <signal.h>

Cgi::Cgi(const Request& req, const std::string& scriptPath, const std::string& serverName, int serverPort) :
	request_(req),
	scriptPath_(scriptPath),
	cgiStatusCode_(200),
	cgiPid_(-1),
	cgiComplete_(false),
	maxFileSize_(10485760),
	inputWritten_(false),
	outputRead_(false),
	serverName_(serverName),
	serverPort_(serverPort) {

}

Cgi::~Cgi() {
	try {
		if (cgiPid_ > 0) {
			// Check if process is still running before trying to kill it
			int status;
			if (waitpid(cgiPid_, &status, WNOHANG) == 0) {
				// Process is still running, kill it
				kill(cgiPid_, SIGTERM);
				waitpid(cgiPid_, NULL, 0);
			}
			cgiPid_ = -1;
		}
		// Close pipes safely
		if (pipeIn_[0] != -1) {
			close(pipeIn_[0]);
			pipeIn_[0] = -1;
		}
		if (pipeIn_[1] != -1) {
			close(pipeIn_[1]);
			pipeIn_[1] = -1;
		}
		if (pipeOut_[0] != -1) {
			close(pipeOut_[0]);
			pipeOut_[0] = -1;
		}
		if (pipeOut_[1] != -1) {
			close(pipeOut_[1]);
			pipeOut_[1] = -1;
		}
	} catch (const std::exception& e) {
		std::cerr << "DEBUG: Error in CGI destructor: " << e.what() << std::endl;
	}
}

Cgi::CgiException::CgiException(const std::string& message) : 
	message_(message) {

}

const char* Cgi::CgiException::what() const throw() {
	return (message_.c_str());
}

Cgi::CgiException::~CgiException() {

}

void Cgi::setScriptPath(const std::string& scriptPath) {
	scriptPath_ = scriptPath;
}

void Cgi::setCgiStatusCode(int statusCode) {
	cgiStatusCode_ = statusCode;
}


int Cgi::getCgiStatusCode() const {
	return (cgiStatusCode_);
}

bool Cgi::isCgiComplete() const {
	return (cgiComplete_);
}

std::string Cgi::getCgiOutput() const {
	return (cgiOutput_);
}

void Cgi::setUploadDir(const std::string& uploadDir) {
	uploadDir_ = uploadDir;
}

void Cgi::setMaxFileSize(size_t maxSize) {
	maxFileSize_ = maxSize;
}

std::unordered_map<std::string, std::string> Cgi::buildEnv() {
	std::unordered_map<std::string, std::string>	env;

	env["REQUEST_METHOD"] = request_.getMethod();
	env["CONTENT_LENGTH"] = std::to_string(request_.getBody().length());
	
	const std::unordered_map<std::string, std::string>& headers = request_.getHeaders();

	if (headers.find("Content-Type") != headers.end())
		env["CONTENT_TYPE"] = headers.at("Content-Type");
	else
		env["CONTENT_TYPE"] = "text/plain"; // Default if not set
	
	env["SCRIPT_NAME"] = scriptPath_;
	env["SERVER_PROTOCOL"] = "HTTP/1.1";
	env["GATEWAY_INTERFACE"] = "Cgi/1.1";
	env["REDIRECT_STATUS"] = "202";
	env["SERVER_SOFTWARE"] = "Webserv42";
	
	// Prevent Python from creating bytecode cache files
	env["PYTHONDONTWRITEBYTECODE"] = "1";
	
	// Add upload-specific environment variables
	if (!uploadDir_.empty()) {
		env["UPLOAD_DIR"] = uploadDir_;
	}
	env["MAX_FILE_SIZE"] = std::to_string(maxFileSize_);
	
	// Add query string if present
	if (request_.getQueryString().empty() == false) {
		env["QUERY_STRING"] = request_.getQueryString();
	}
	
	// Add request URI
	env["REQUEST_URI"] = request_.getPath();
	
	// Add server name and port
	env["SERVER_NAME"] = serverName_.empty() ? "localhost" : serverName_;
	env["SERVER_PORT"] = serverPort_ > 0 ? std::to_string(serverPort_) : "80";
	
	return (env);
}

void	Cgi::startCgi(std::function<void(int, uint32_t)> addtoEpoll) {
	if (pipe(pipeIn_) == -1 || pipe(pipeOut_) == -1) {
		throw CgiException("Cgi: Pipe creation failed");
	}
	
	// Start the CGI process
	cgiPid_ = fork();
	if (cgiPid_ < 0) {
		throw CgiException("Cgi: Fork failed");
	}

	// child
	if (cgiPid_ == 0) {
		dup2(pipeIn_[0], STDIN_FILENO);
		dup2(pipeOut_[1], STDOUT_FILENO);

		close(pipeIn_[1]);
		close(pipeOut_[0]);

		std::unordered_map<std::string, std::string>	env = buildEnv();
		char* envp[env.size() + 1];
		size_t i = 0;
		std::vector<std::string> envPart;
		for (const auto& value : env) {
			envPart.push_back(value.first + "=" + value.second);
			envp[i++] = const_cast<char*>(envPart.back().c_str());
		}
		envp[i] = NULL;

		std::string prePath = scriptPath_;
		char *path = const_cast<char*>(prePath.c_str());
		char *argvPath = const_cast<char*>(scriptPath_.c_str());

		char* argv[] = {argvPath, NULL};
		execve(path, argv, envp);
		
		perror("execve");
		exit(1);
	}

	// parent
	close(pipeIn_[0]);
	close(pipeOut_[1]);
	
	// Add pipes to epoll for non-blocking I/O
	addtoEpoll(pipeOut_[0], EPOLLIN | EPOLLRDHUP | EPOLLET);
	if (request_.getMethod() == "POST" && !request_.getBody().empty()) {
		addtoEpoll(pipeIn_[1], EPOLLOUT | EPOLLRDHUP | EPOLLET);
	}
}

std::string Cgi::runCgi() {
	std::cout << "DEBUG: runCgi called for script: " << scriptPath_ << std::endl;
	
	// for POST, write body to CGI input
	if (request_.getMethod() == "POST" && !request_.getBody().empty()) {
		std::string body = request_.getBody();
		std::cout << "DEBUG: Writing POST data to CGI input, size: " << body.length() << " bytes" << std::endl;
		ssize_t written = write(pipeIn_[1], body.c_str(), body.length());
		if (written == -1) {
			close(pipeIn_[1]);
			throw CgiException("Cgi: Failed to write POST data to CGI input");
		}
		std::cout << "DEBUG: Written " << written << " bytes to CGI input" << std::endl;
	}
	close(pipeIn_[1]);

	// Read CGI output - this is now blocking but we'll make it non-blocking later
	std::string	output;
	char buffer[4096];
	ssize_t n;
	std::cout << "DEBUG: Reading CGI output" << std::endl;
	
	// Read all available data from CGI output
	while ((n = read(pipeOut_[0], buffer, sizeof(buffer))) > 0) {
		output.append(buffer, n);
		std::cout << "DEBUG: Read " << n << " bytes from CGI, total: " << output.length() << std::endl;
	}
	close(pipeOut_[0]);

	std::cout << "DEBUG: Waiting for CGI process to finish" << std::endl;
	int status;
	waitpid(cgiPid_, &status, 0);
	cgiPid_ = -1;
	std::cout << "DEBUG: CGI process finished with status: " << status << std::endl;
	
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		std::cerr << "DEBUG: CGI process failed with exit code: " << WEXITSTATUS(status) << std::endl;
		cgiStatusCode_ = WEXITSTATUS(status);
		throw CgiException("Cgi: Script execution failed");
	}

	if (output.empty()) {
		std::cerr << "DEBUG: CGI produced empty output" << std::endl;
		cgiStatusCode_ = 500;
		throw CgiException("Cgi: Empty output_");
	}

	std::cout << "DEBUG: CGI script completed successfully. Output preview: " << output.substr(0, 100) << "..." << std::endl;
	cgiStatusCode_ = 200;
	return (output);
}

