#include "Cgi.hpp"

Cgi::Cgi(const Request& req, const std::string& scriptPath) :
	request_(req),
	scriptPath_(scriptPath),
	cgiStatusCode_(200) {

}

Cgi::~Cgi() {

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

std::unordered_map<std::string, std::string> Cgi::buildEnv() {

	std::unordered_map<std::string, std::string>	env;

	env["REQUEST_METHOD"] = request_.getMethod();
	env["CONTENT_LENGTH"] = std::to_string(request_.getBody().length());
	
	const std::unordered_map<std::string, std::string>& headers = request_.getHeaders();

	if (headers.find("Content-Type") != headers.end())
		env["CONTENT_TYPE"] = headers.at("Content-Type");
	else
		env["CONTENT_TYPE"] = "text/plain"; // Default if not set
	
	env["SCRIPT_NAME"] = scriptPath_;;
	env["SERVER_PROTOCOL"] = "HTTP/1.1";
	env["GATEWAY_INTERFACE"] = "Cgi/1.1";
	// env["PATH_INFO"] = ;
	env["REDIRECT_STATUS"] = "202";
	env["SERVER_SOFTWARE"] = "Webserv42";

	// // Headers jako HTTP_*
	// const std::unordered_map<std::string, std::string>& headers = request_.getHeaders();
	// for (std::unordered_map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
	// 	std::string key = it->first;
	// 	std::string val = it->second;

	// 	std::string envKey = "HTTP_";
	// 	for (size_t i = 0; i < key.length(); ++i) {
	// 		char c = key[i];
	// 		if (c == '-') envKey += '_';
	// 		else envKey += toupper(c);
	// 	}
	// 	env[envKey] = val;
	// }
	return (env);
}

void	Cgi::startCgi(std::function<void(int, uint32_t)> addtoEpoll) {
	if (pipe(pipeIn_) == -1 || pipe(pipeOut_) == -1)
		throw CgiException("Cgi: Pipe creation failed");
	
	addtoEpoll(pipeIn_[0], EPOLLIN | EPOLLRDHUP | EPOLLET);
	addtoEpoll(pipeOut_[1], EPOLLOUT | EPOLLRDHUP | EPOLLET);
}

std::string Cgi::runCgi() {
	
	std::cout << "Running CGI script: " << scriptPath_ << std::endl;
	pid_t pid = fork();
	if (pid < 0)
		throw CgiException("Cgi: Fork failed");

	// child
	if (pid == 0) {
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

		//TODO -> get rid of the hardcoded www
		std::string prePath = scriptPath_;
		char *path = const_cast<char*>(prePath.c_str());
		char *argvPath = const_cast<char*>(scriptPath_.c_str());
		std::cerr << "path = " << path << std::endl;

		char* argv[] = {argvPath, NULL};
		std::cerr << "argv[0] = " << argv[0] << std::endl;
		execve(path, argv, envp);
		
		perror("execve");
		exit(1);
	}

	// parent
	close(pipeIn_[0]);
	close(pipeOut_[1]);

	// for POST, write body to CGI input
	if (request_.getMethod() == "POST" && !request_.getBody().empty()) {
		std::string body = request_.getBody();
		write(pipeIn_[1], body.c_str(), body.length());
	}
	close(pipeIn_[1]);

	//TODO fix the response
	std::string	output;
	char buffer[4096];
	ssize_t n;
	while ((n = read(pipeOut_[0], buffer, sizeof(buffer))) > 0) {
		output.append(buffer, n);
	}
	close(pipeOut_[0]);

	int status;
	waitpid(pid, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		cgiStatusCode_ = WEXITSTATUS(status);
		throw CgiException("Cgi: Script execution failed");
	}

	if (output.empty())
	{
		cgiStatusCode_ = 500;
		throw CgiException("Cgi: Empty output_");
	}


	std::cout << "CGI script output: " << output << std::endl;
	cgiStatusCode_ = 200;
	return (output);
}
