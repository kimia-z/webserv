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
	startTime_(time(NULL)),
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

void Cgi::createPipes() {
	// Create pipes for communication with CGI process
	if (pipe(pipeIn_) == -1 || pipe(pipeOut_) == -1) {
		throw CgiException("Cgi: Pipe creation failed");
	}
}

void Cgi::forkCgiProcess() {
	// Fork the process
	cgiPid_ = fork();
	if (cgiPid_ < 0) {
		throw CgiException("Cgi: Fork failed");
	}

	// Child process - execute the CGI script
	if (cgiPid_ == 0) {
		// Redirect stdin and stdout to pipes
		dup2(pipeIn_[0], STDIN_FILENO);
		dup2(pipeOut_[1], STDOUT_FILENO);

		// Close unused pipe ends in child
		close(pipeIn_[1]);
		close(pipeOut_[0]);

		// Build environment variables
		std::unordered_map<std::string, std::string> env = buildEnv();
		char* envp[env.size() + 1];
		size_t i = 0;
		std::vector<std::string> envPart;
		for (const auto& value : env) {
			envPart.push_back(value.first + "=" + value.second);
			envp[i++] = const_cast<char*>(envPart.back().c_str());
		}
		envp[i] = NULL;

		// Prepare arguments for execve
		std::string prePath = scriptPath_;
		char *path = const_cast<char*>(prePath.c_str());
		char *argvPath = const_cast<char*>(scriptPath_.c_str());
		char* argv[] = {argvPath, NULL};

		// Execute the CGI script
		execve(path, argv, envp);
		
		// If execve fails, exit with error
		perror("execve");
		exit(1);
	}
}

void Cgi::setupCgiPipes(std::function<void(int, uint32_t)> addtoEpoll) {
	// Close unused pipe ends in parent
	close(pipeIn_[0]);
	close(pipeOut_[1]);

	// Add output pipe to epoll for reading CGI output
	addtoEpoll(pipeOut_[0], EPOLLIN | EPOLLRDHUP | EPOLLET);

	// Handle input pipe based on request method
	if (request_.getMethod() == "POST" && !request_.getBody().empty()) {
		// For POST with body, add input pipe to epoll for writing
		addtoEpoll(pipeIn_[1], EPOLLOUT | EPOLLRDHUP | EPOLLET);
	} else {
		// For GET requests or empty POST, close input pipe immediately
		close(pipeIn_[1]);
		pipeIn_[1] = -1;
		inputWritten_ = true;
	}
}

// change: CGI not synchronous anymore
void Cgi::startCgi(std::function<void(int, uint32_t)> addtoEpoll) {
	// Step 1: Create pipes for communication
	createPipes();
	
	// Step 2: Fork the process and execute CGI script
	forkCgiProcess();
	
	// Step 3: Setup pipes for non-blocking I/O
	setupCgiPipes(addtoEpoll);
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

// change: non-blocking server when writing large POST data to CGI processes
bool Cgi::writeToCgiInput() {
	if (inputWritten_ || pipeIn_[1] == -1) {
		return true; // Already written or pipe closed
	}

	if (request_.getMethod() == "POST" && !request_.getBody().empty()) {
		const std::string& body = request_.getBody();
		ssize_t written = write(pipeIn_[1], body.c_str(), body.length());

		if (written == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				return false; // Would block, try again later
			} else {
				// Error occurred, but mark as complete
				close(pipeIn_[1]);
				pipeIn_[1] = -1;
				inputWritten_ = true;
				return true;
			}
		} else if (written == static_cast<ssize_t>(body.length())) {
			// All data written successfully
			close(pipeIn_[1]);
			pipeIn_[1] = -1;
			inputWritten_ = true;
			return true;
		} else {
			// Partial write - mark as complete
			close(pipeIn_[1]);
			pipeIn_[1] = -1;
			inputWritten_ = true;
			return true;
		}
	} else {
		// No input to write
		close(pipeIn_[1]);
		pipeIn_[1] = -1;
		inputWritten_ = true;
		return true;
	}
}

// change: reading CGI output in chunks without blocking the server
bool Cgi::readFromCgiOutput() {
	if (outputRead_ || pipeOut_[0] == -1) {
		return true; // Already read or pipe closed
	}

	char buffer[4096];
	ssize_t n = read(pipeOut_[0], buffer, sizeof(buffer));

	if (n > 0) {
		cgiOutput_.append(buffer, n);
		return false; // More data might be available, keep pipe in epoll
	} else if (n == 0) {
		// EOF - no more data, pipe closed by CGI process
		close(pipeOut_[0]);
		pipeOut_[0] = -1;
		outputRead_ = true;
		// change: Mark CGI as complete when EOF is reached (based on external repo analysis)
		if (!cgiComplete_) {
			cgiComplete_ = true;
			cgiPid_ = -1;
		}
		return true;
	} else {
		// Error or would block
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return false; // Would block, try again later
		} else {
			// Error occurred, but mark as complete anyway
			close(pipeOut_[0]);
			pipeOut_[0] = -1;
			outputRead_ = true;
			return true;
		}
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
		cgiComplete_ = true;

		if (WIFEXITED(status)) {
			cgiStatusCode_ = WEXITSTATUS(status);
		} else {
			cgiStatusCode_ = 500; // Process terminated abnormally
		}
		return true;
	} else {
		// Error in waitpid
		cgiPid_ = -1;
		cgiComplete_ = true;
		cgiStatusCode_ = 500;
		return true; // Mark as complete even on error
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

time_t Cgi::getStartTime() const {
	return startTime_;
}

