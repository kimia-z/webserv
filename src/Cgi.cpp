#include "Cgi.hpp"

Cgi::Cgi(const Request& req, const Location& loc) :
	request_(req),
	location_(loc),
	env_(), // environment variables for CGI
	output_(), // output from the CGI script
	body_(), // body of the response
	contentType_("text/plain"),
	statusCode_(200) {

	scriptPath_ = getScriptPath(); // e.g. /www/Cgi-bin/test.py
}

Cgi::~Cgi() {}

std::string Cgi::getScriptPath() const {
	std::string root = location_.getRoot(); // e.g. /www
	// std::string uri = request_.getUri(); // e.g. /Cgi-bin/test.py
	// std::string path = root + uri; // /www/Cgi-bin/test.py
	return (root); //TODO delete it - tmp rubbish
	// return (path);
}								

void Cgi::buildEnv() {
	env_["REQUEST_METHOD"] = request_.getMethod();
	// env_["QUERY_STRING"] = request_.getQueryString();
	// TODO: create the getHeader(std::string key)
	// env_["CONTENT_LENGTH"] = request_.getHeader("Content-Length");
	// env_["CONTENT_TYPE"] = request_.getHeader("Content-Type");
	// env_["SCRIPT_NAME"] = request_.getUri();
	env_["SERVER_PROTOCOL"] = "HTTP/1.1";
	env_["GATEWAY_INTERFACE"] = "Cgi/1.1";
	// env_["PATH_INFO"] = request_.getUri();
	env_["REDIRECT_STATUS"] = "200";
	env_["SERVER_SOFTWARE"] = "Webserv42";

	// Headers jako HTTP_*
	const std::unordered_map<std::string, std::string>& headers = request_.getHeaders();
	for (std::unordered_map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
		std::string key = it->first;
		std::string val = it->second;

		std::string envKey = "HTTP_";
		for (size_t i = 0; i < key.length(); ++i) {
			char c = key[i];
			if (c == '-') envKey += '_';
			else envKey += toupper(c);
		}
		env_[envKey] = val;
	}
}

std::string Cgi::runCgi() {
	int inPipe[2];
	int outPipe[2];
	if (pipe(inPipe) == -1 || pipe(outPipe) == -1)
		throw CgiException("Cgi: Pipe creation failed");

	pid_t pid = fork();
	if (pid < 0)
		throw CgiException("Cgi: Fork failed");

	if (pid == 0) {
		// child
		dup2(inPipe[0], STDIN_FILENO);
		dup2(outPipe[1], STDOUT_FILENO);

		close(inPipe[1]);
		close(outPipe[0]);

		char** envp = new char*[env_.size() + 1];
		size_t i = 0;
		for (std::map<std::string, std::string>::iterator it = env_.begin(); it != env_.end(); ++it) {
			std::string pair = it->first + "=" + it->second;
			envp[i] = strdup(pair.c_str());
			++i;
		}
		envp[i] = NULL;

		char* argv[] = { const_cast<char*>(scriptPath_.c_str()), NULL };
		execve(scriptPath_.c_str(), argv, envp);

		// Błąd execve
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

	char buffer[1024];
	ssize_t bytesRead;
	while ((bytesRead = read(outPipe[0], buffer, sizeof(buffer))) > 0) {
		output_.append(buffer, bytesRead);
	}
	close(outPipe[0]);

	int status;
	waitpid(pid, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		throw CgiException("Cgi: Script execution failed");

	if (output_.empty())
		throw CgiException("Cgi: Empty output_");

	parseCgiOutput();
	return (body_);
}

void Cgi::parseCgiOutput() {
	size_t sep = output_.find("\r\n\r\n");
	if (sep == std::string::npos)
		throw CgiException("Cgi: Missing header-body separator");

	std::string headerPart = output_.substr(0, sep);
	body_ = output_.substr(sep + 4);

	std::istringstream headers(headerPart);
	std::string line;
	while (std::getline(headers, line)) {
		if (line.find("Content-Type:") == 0) {
			contentType_ = line.substr(strlen("Content-Type:"));
			while (!contentType_.empty() && (contentType_[0] == ' ' || contentType_[0] == '\r'))
				contentType_.erase(0, 1);
		}
	}

	if (contentType_.empty())
		contentType_ = "text/plain";
}

int Cgi::getStatusCode() const {
	return (statusCode_);
}

std::string Cgi::getContentType() const {
	return (contentType_);
}