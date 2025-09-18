#include "Cgi.hpp"
#include <signal.h>

Cgi::Cgi(const Request& req, const std::string& scriptPath, const std::string& serverName, int serverPort) :
	request_(req),
	scriptPath_(scriptPath),
	cgiStatusCode_(200),
	cgiPid_(-1),
	isCgiComplete_(false),
	maxFileSize_(10485760),
	inputWritten_(false),
	outputRead_(false),
	bodyOffset_(0),
	startTime_(time(NULL)),
	serverName_(serverName),
	serverPort_(serverPort) {

}

Cgi::~Cgi() {
	try {
		if (cgiPid_ > 0) {
			int status;
			if (waitpid(cgiPid_, &status, WNOHANG) == 0) {
				kill(cgiPid_, SIGTERM);
				waitpid(cgiPid_, NULL, 0);
			}
			cgiPid_ = -1;
		}
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

int Cgi::getInputPipe() const {
	return pipeIn_[1];
}

int Cgi::getOutputPipe() const {
	return pipeOut_[0];
}

int Cgi::getCgiStatusCode() const {
	return (cgiStatusCode_);
}

std::string	Cgi::getCgiOutput() const {
	return (cgiOutput_);
}

bool Cgi::getIsCgiComplete() const {
	return (isCgiComplete_);
}

time_t Cgi::getStartTime() const {
	return startTime_;
}

bool Cgi::getIsInputComplete() const {
	return inputWritten_;
}

bool Cgi::getIsOutputComplete() const {
	return outputRead_;
}

void	Cgi::setScriptPath(const std::string& scriptPath) {
	scriptPath_ = scriptPath;
}

void	Cgi::setCgiStatusCode(int statusCode) {
	cgiStatusCode_ = statusCode;
}

void	Cgi::setUploadDir(const std::string& uploadDir) {
	uploadDir_ = uploadDir;
}

void	Cgi::setMaxFileSize(size_t maxSize) {
	maxFileSize_ = maxSize;
}

void	Cgi::markOutputComplete() {
	outputRead_ = true;
}

void	Cgi::forceComplete() {
	isCgiComplete_ = true;
	cgiPid_ = -1;
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
	if (!uploadDir_.empty()) {
		env["UPLOAD_DIR"] = uploadDir_;
	}
	env["MAX_FILE_SIZE"] = std::to_string(maxFileSize_);
	if (request_.getQueryString().empty() == false) {
		env["QUERY_STRING"] = request_.getQueryString();
	}
	env["REQUEST_URI"] = request_.getPath();
	env["SERVER_NAME"] = serverName_.empty() ? "localhost" : serverName_;
	env["SERVER_PORT"] = serverPort_ > 0 ? std::to_string(serverPort_) : "80";
	if (request_.getMethod() == "DELETE") {
		const auto& queryParams = request_.getQueryParams();

		if (queryParams.find("file") != queryParams.end()) {
			env["DELETE_TARGET"] = queryParams.at("file");
		} else if (!request_.getPath().empty()) {
			env["DELETE_TARGET"] = request_.getPath();
		} else { 
			std::string	path = request_.getPath();
			size_t lastSlash = path.find_last_of('/');
			if (lastSlash != std::string::npos && lastSlash < path.length() - 1) {
				env["DELETE_TARGET"] = path.substr(lastSlash + 1);
			} else {
				env["DELETE_TARGET"] = path;
			}
		}
	}
	return (env);
}

void	Cgi::createPipes() {
	if (pipe(pipeIn_) == -1 || pipe(pipeOut_) == -1) {
		throw CgiException("Cgi: Pipe creation failed");
	}
}

void	Cgi::forkCgiProcess() {
	cgiPid_ = fork();
	if (cgiPid_ < 0) {
		throw CgiException("Cgi: Fork failed");
	}
	if (cgiPid_ == 0) {
		dup2(pipeIn_[0], STDIN_FILENO);
		dup2(pipeOut_[1], STDOUT_FILENO);

		close(pipeIn_[1]);
		close(pipeOut_[0]);

		std::unordered_map<std::string, std::string> env = buildEnv();
		std::vector<std::string> envStrings;
		std::vector<char*> envp;

		for (const auto& value : env) {
			envStrings.push_back(value.first + "=" + value.second);
		}
		for (auto& str : envStrings) {
			envp.push_back(const_cast<char*>(str.c_str()));
		}
		envp.push_back(NULL);

		std::string	prePath = scriptPath_;
		char *path = const_cast<char*>(prePath.c_str());
		char *argvPath = const_cast<char*>(scriptPath_.c_str());
		char* argv[] = {argvPath, NULL};

		execve(path, argv, envp.data());
		
		perror("execve");
		exit(1);
	}
}

void	Cgi::setupCgiPipes(std::function<void(int, uint32_t)> addtoEpoll) {
	std::cout << "DEBUG: setupCgiPipes called" << std::endl;
	std::cout << "DEBUG: Method: " << request_.getMethod() << std::endl;
	std::cout << "DEBUG: Body empty: " << (request_.getBody().empty() ? "YES" : "NO") << std::endl;
	std::cout << "DEBUG: Body length: " << request_.getBody().length() << std::endl;

	close(pipeIn_[0]);
	close(pipeOut_[1]);

	// Add output pipe to epoll for reading CGI output
	addtoEpoll(pipeOut_[0], EPOLLIN | EPOLLRDHUP | EPOLLET);
	std::cout << "DEBUG: Added output pipe: " << pipeOut_[0] << " to epoll" << std::endl;

	if (request_.getMethod() == "POST" && !request_.getBody().empty()) {
		// For POST with body, add input pipe to epoll for writing
		addtoEpoll(pipeIn_[1], EPOLLOUT | EPOLLRDHUP | EPOLLET);
		std::cout << "DEBUG: Added input pipe: " << pipeIn_[1] << " to epoll for POST with body" << std::endl;
	} else {
		// For GET requests or empty POST, close input pipe immediately
		close(pipeIn_[1]);
		pipeIn_[1] = -1;
		inputWritten_ = true;
		std::cout << "DEBUG: Closed input pipe for GET/empty POST" << std::endl;
	}
}

void	Cgi::startCgi(std::function<void(int, uint32_t)> addtoEpoll) {
	bodyOffset_ = 0;
	createPipes();
	forkCgiProcess();	
	setupCgiPipes(addtoEpoll);
}

std::string	Cgi::runCgi() {
	if (request_.getMethod() == "POST" && !request_.getBody().empty()) {
		std::string	body = request_.getBody();
		ssize_t written = write(pipeIn_[1], body.c_str(), body.length());
		if (written == -1) {
			close(pipeIn_[1]);
			throw CgiException("Cgi: Failed to write POST data to CGI input");
		}
	}
	close(pipeIn_[1]);

	std::string	output;
	char buffer[BUFSIZ];
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
	std::cout << "DEBUG: writeToCgiInput() called" << std::endl;
	std::cout << "DEBUG: Method: " << request_.getMethod() << std::endl;
	std::cout << "DEBUG: Body length: " << request_.getBody().length() << std::endl;
	std::cout << "DEBUG: BodyOffset: " << bodyOffset_ << std::endl;

	std::cout << "DEBUG: writeToCgiInput() called, bodyOffset_: " << bodyOffset_ << std::endl;

	if (inputWritten_ || pipeIn_[1] == -1) {
		std::cout << "DEBUG: Input already written or pipe closed" << std::endl;
		return (true); // Already written or pipe closed
	}
	// size_t writeSize;
	if (request_.getMethod() == "POST" && !request_.getBody().empty()) {
		const std::string& body = request_.getBody();
		std::cout << "DEBUG: POST body length: " << body.length() << std::endl;

		size_t remainingBytes = body.length() - bodyOffset_;
		std::cout << "DEBUG: Remaining bytes: " << remainingBytes << std::endl;
		if (remainingBytes == 0) {
			std::cout << "DEBUG: All data written, closing pipe" << std::endl;
			close(pipeIn_[1]);
			pipeIn_[1] = -1;
			inputWritten_ = true;
			return (true);
		}
		size_t writeSize = std::min(remainingBytes, static_cast<size_t>(BUFSIZ));
		std::cout << "DEBUG: Writing " << writeSize << " bytes from offset" << std::endl;
		ssize_t written = write(pipeIn_[1], body.c_str() + bodyOffset_, writeSize);
		std::cout << "DEBUG: Written " << written << " bytes" << std::endl;
		if (written == -1) {
			std::cout << "DEBUG: Write error: " << strerror(errno) << std::endl;
			close(pipeIn_[1]);
			pipeIn_[1] = -1;
			inputWritten_ = true;
			return (true); // Error writing, consider input done
		}
		else if (written > 0) {
			bodyOffset_ += written;
			std::cout << "DEBUG: New bodyOffset: " << bodyOffset_ << std::endl;

			if (bodyOffset_ >= body.length()) {
				std::cout << "DEBUG: All data written, closing pipe" << std::endl;
				close(pipeIn_[1]);
				pipeIn_[1] = -1;
				inputWritten_ = true;
				return (true);
			} else {
				// Partial write, adjust body and try again later
				std::cout << "DEBUG: More data to write" << std::endl;
				return (false); // More data to write
			}
		}
	} else {
		// For GET requests or empty POST, close input pipe immediately
		std::cout << "DEBUG: GET or empty POST, closing pipe" << std::endl;
		close(pipeIn_[1]);
		pipeIn_[1] = -1;
		inputWritten_ = true;
		return (true);
	}
	return (false);
}

bool Cgi::readFromCgiOutput() {
	if (outputRead_ || pipeOut_[0] == -1) {
		return true; // Already read or pipe closed
	}

	char buffer[BUFSIZ];
	ssize_t n = read(pipeOut_[0], buffer, sizeof(buffer));

	if (n > 0) {
		cgiOutput_.append(buffer, n);
		return (false); // More data might be available, keep pipe in epoll
	} else if (n == 0) {
		close(pipeOut_[0]);
		pipeOut_[0] = -1;
		outputRead_ = true;
		if (!isCgiComplete_) {
			markOutputComplete();
			cgiPid_ = -1;
		}
		return true;
	} else {
		// Error occurred, but mark as complete anyway
		close(pipeOut_[0]);
		pipeOut_[0] = -1;
		outputRead_ = true;
		return true;
	}
}

bool Cgi::checkCgiProcess() {
	if (cgiPid_ <= 0) {
		return true; // Process already finished or not started
	}

	int status;
	pid_t result = waitpid(cgiPid_, &status, WNOHANG);

	if (result == 0) {
		return false; // Process still running
	} else if (result == cgiPid_) {
		// Process finished
		cgiPid_ = -1;
		isCgiComplete_ = true;

		if (WIFEXITED(status)) {
			cgiStatusCode_ = WEXITSTATUS(status);
		} else {
			cgiStatusCode_ = 500; // Process terminated abnormally
		}
		return true;
	} else {
		// Error in waitpid
		cgiPid_ = -1;
		isCgiComplete_ = true;
		cgiStatusCode_ = 500;
		return true; // Mark as complete even on error
	}
}



