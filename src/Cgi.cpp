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
	// std::cout << "DEBUG: startCgi called for script: " << scriptPath_ << std::endl;
	
	if (pipe(pipeIn_) == -1 || pipe(pipeOut_) == -1) {
		throw CgiException("Cgi: Pipe creation failed");
	}
	
	// std::cout << "DEBUG: Created pipes - input: [" << pipeIn_[0] << "," << pipeIn_[1] << "], output: [" << pipeOut_[0] << "," << pipeOut_[1] << "]" << std::endl;
	
	// Start the CGI process
	cgiPid_ = fork();
	if (cgiPid_ < 0) {
		throw CgiException("Cgi: Fork failed");
	}
	
	// std::cout << "DEBUG: Forked process with PID: " << cgiPid_ << std::endl;

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
	
	// std::cout << "DEBUG: Parent process - closed child-side pipes" << std::endl;
	
	// Add pipes to epoll for non-blocking I/O
	addtoEpoll(pipeOut_[0], EPOLLIN | EPOLLRDHUP | EPOLLET);
	// std::cout << "DEBUG: Added output pipe " << pipeOut_[0] << " to epoll" << std::endl;
	
	if (request_.getMethod() == "POST" && !request_.getBody().empty()) {
		addtoEpoll(pipeIn_[1], EPOLLOUT | EPOLLRDHUP | EPOLLET);
		// std::cout << "DEBUG: Added input pipe " << pipeIn_[1] << " to epoll for POST data" << std::endl;
	} else {
		// For GET requests or empty POST, close input pipe immediately
		close(pipeIn_[1]);
		pipeIn_[1] = -1;
		inputWritten_ = true;
		// std::cout << "DEBUG: Closed input pipe immediately for GET/empty POST" << std::endl;
	}
}

std::string Cgi::runCgi() {
	// for POST, write body to CGI input
	if (request_.getMethod() == "POST" && !request_.getBody().empty()) {
		std::string body = request_.getBody();
		ssize_t written = write(pipeIn_[1], body.c_str(), body.length());
		if (written == -1) {
			close(pipeIn_[1]);
			throw CgiException("Cgi: Failed to write POST data to CGI input");
		}
	}
	close(pipeIn_[1]);

	std::string	output;
	char buffer[4096];
	ssize_t n;
	
	while ((n = read(pipeOut_[0], buffer, sizeof(buffer))) > 0) {
		output.append(buffer, n);
	}
	close(pipeOut_[0]);

	int status;
	waitpid(cgiPid_, &status, 0);
	cgiPid_ = -1;
	
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		cgiStatusCode_ = WEXITSTATUS(status);
		throw CgiException("Cgi: Script execution failed");
	}

	if (output.empty()) {
		cgiStatusCode_ = 500;
		throw CgiException("Cgi: Empty output_");
	}

	cgiStatusCode_ = 200;
	return (output);
}

bool Cgi::writeToCgiInput() {
	// std::cout << "DEBUG: writeToCgiInput called, inputWritten_=" << inputWritten_ << ", pipeIn_[1]=" << pipeIn_[1] << std::endl;
	
	if (inputWritten_ || pipeIn_[1] == -1) {
		// std::cout << "DEBUG: Input already written or pipe closed, returning true" << std::endl;
		return true; // Already written or pipe closed
	}
	
	if (request_.getMethod() == "POST" && !request_.getBody().empty()) {
		const std::string& body = request_.getBody();
		// std::cout << "DEBUG: Writing " << body.length() << " bytes to CGI input" << std::endl;
		ssize_t written = write(pipeIn_[1], body.c_str(), body.length());
		
		if (written == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				// std::cout << "DEBUG: Write would block, returning false" << std::endl;
				return false; // Would block, try again later
			} else {
				// std::cout << "DEBUG: Write error: " << strerror(errno) << std::endl;
				close(pipeIn_[1]);
				pipeIn_[1] = -1;
				return false; // Error occurred
			}
		} else if (written == static_cast<ssize_t>(body.length())) {
			// std::cout << "DEBUG: All input written successfully" << std::endl;
			// All data written successfully
			close(pipeIn_[1]);
			pipeIn_[1] = -1;
			inputWritten_ = true;
			return true;
		} else {
			// std::cout << "DEBUG: Partial write, closing pipe" << std::endl;
			// Partial write - for simplicity, we close the pipe
			close(pipeIn_[1]);
			pipeIn_[1] = -1;
			inputWritten_ = true;
			return true;
		}
	} else {
		// std::cout << "DEBUG: No POST body, closing input pipe" << std::endl;
		// No input to write
		close(pipeIn_[1]);
		pipeIn_[1] = -1;
		inputWritten_ = true;
		return true;
	}
}

bool Cgi::readFromCgiOutput() {
	// std::cout << "DEBUG: readFromCgiOutput called, outputRead_=" << outputRead_ << ", pipeOut_[0]=" << pipeOut_[0] << std::endl;
	
	if (outputRead_ || pipeOut_[0] == -1) {
		// std::cout << "DEBUG: Output already read or pipe closed, returning true" << std::endl;
		return true; // Already read or pipe closed
	}
	
	char buffer[4096];
	ssize_t n = read(pipeOut_[0], buffer, sizeof(buffer));
	
	if (n > 0) {
		// std::cout << "DEBUG: Read " << n << " bytes from CGI output" << std::endl;
		cgiOutput_.append(buffer, n);
		return false; // More data might be available
	} else if (n == 0) {
		// std::cout << "DEBUG: EOF reached, all output read" << std::endl;
		// EOF - no more data
		close(pipeOut_[0]);
		pipeOut_[0] = -1;
		outputRead_ = true;
		return true;
	} else {
		// Error or would block
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			// std::cout << "DEBUG: Read would block, returning false" << std::endl;
			return false; // Would block, try again later
		} else {
			// std::cout << "DEBUG: Read error: " << strerror(errno) << std::endl;
			// Even on error, if we have some output, consider it complete
			close(pipeOut_[0]);
			pipeOut_[0] = -1;
			outputRead_ = true;
			return true; // Consider it complete even on error
		}
	}
}

bool Cgi::checkCgiProcess() {
	// std::cout << "DEBUG: checkCgiProcess called, cgiPid_=" << cgiPid_ << std::endl;
	
	if (cgiPid_ <= 0) {
		// std::cout << "DEBUG: Process already finished or not started" << std::endl;
		return true; // Process already finished or not started
	}
	
	int status;
	pid_t result = waitpid(cgiPid_, &status, WNOHANG);
	
	if (result == 0) {
		// std::cout << "DEBUG: Process still running" << std::endl;
		return false; // Process still running
	} else if (result == cgiPid_) {
		// std::cout << "DEBUG: Process finished with status=" << status << std::endl;
		// Process finished
		cgiPid_ = -1;
		cgiComplete_ = true;
		
		if (WIFEXITED(status)) {
			cgiStatusCode_ = WEXITSTATUS(status);
			// std::cout << "DEBUG: Process exited normally with code=" << cgiStatusCode_ << std::endl;
			if (cgiStatusCode_ != 0) {
				return false; // CGI failed
			}
		} else {
			// std::cout << "DEBUG: Process terminated abnormally" << std::endl;
			cgiStatusCode_ = 500; // Process terminated abnormally
			return false;
		}
		return true;
	} else {
		// std::cout << "DEBUG: waitpid error: " << strerror(errno) << std::endl;
		// Error in waitpid
		cgiPid_ = -1;
		cgiComplete_ = true;
		cgiStatusCode_ = 500;
		return false;
	}
}

bool Cgi::isInputComplete() const {
	return inputWritten_;
}

bool Cgi::isOutputComplete() const {
	return outputRead_;
}

int Cgi::getInputPipe() const {
	return pipeIn_[1];
}

int Cgi::getOutputPipe() const {
	return pipeOut_[0];
}

void Cgi::markOutputComplete() {
	outputRead_ = true;
}

void Cgi::forceComplete() {
	cgiComplete_ = true;
	cgiPid_ = -1;
}

