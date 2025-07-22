#include "Cgi.hpp"

Cgi::Cgi(const Request& req, const std::string& scriptPath) :
	request_(req),
	scriptPath_(scriptPath) {

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

std::string Cgi::runCgi() {
	int inPipe[2];
	int outPipe[2];
	if (pipe(inPipe) == -1 || pipe(outPipe) == -1)
		throw CgiException("Cgi: Pipe creation failed");

	pid_t pid = fork();
	if (pid < 0)
		throw CgiException("Cgi: Fork failed");

	// child
	if (pid == 0) {
		dup2(inPipe[0], STDIN_FILENO);
		dup2(outPipe[1], STDOUT_FILENO);

		close(inPipe[1]);
		close(outPipe[0]);

		std::unordered_map<std::string, std::string>	env = buildEnv();
		char* envp[env.size() + 1];
		size_t i = 0;
		std::vector<std::string> envPart;
		for (const auto& value : env) {
			envPart.push_back(value.first + "=" + value.second);
			envp[i++] = const_cast<char*>(envPart.back().c_str());
		}
		envp[i] = NULL;

		char* argv[] = { const_cast<char*>(scriptPath_.c_str()), NULL };
		execve(scriptPath_.c_str(), argv, envp);
		
		perror("execve");
		exit(1);
	}

	// parent
	close(inPipe[0]);
	close(outPipe[1]);

	// for POST, write body to CGI input
	if (request_.getMethod() == "POST" && !request_.getBody().empty()) {
		std::string body = request_.getBody();
		write(inPipe[1], body.c_str(), body.length());
	}
	close(inPipe[1]);

	std::string	output;
	char buffer[4096];
	ssize_t bytesRead;
	while ((bytesRead = read(outPipe[0], buffer, sizeof(buffer))) > 0) {
		output.append(buffer, bytesRead);
	}
	close(outPipe[0]);

	int status;
	waitpid(pid, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		throw CgiException("Cgi: Script execution failed");

	if (output.empty())
		throw CgiException("Cgi: Empty output_");

	return (output);
}
