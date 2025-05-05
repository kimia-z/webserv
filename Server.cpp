#include <iostream>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>

#define DOMAIN PF_INET6
#define SOCKET_TYPE SOCK_STREAM
#define PROTOCOL 0
#define PORT "8081"
#define INTERFACE INADDR_ANY
#define BACKLOG 10
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"

int main()
{
	int status;
	struct addrinfo hints, *res, *p;
	struct sockaddr_storage other_addr;
	char ipstr[INET6_ADDRSTRLEN];
	socklen_t addr_size;
	//"res" will point to the results

	memset(&hints, 0 , sizeof(hints)); // make sure the struct is empty
	hints.ai_family = AF_UNSPEC;      // don't care if IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;  // TCP stream socket
	hints.ai_flags = AI_PASSIVE;      // fill in my IP for me???

	if ((status = getaddrinfo(NULL, PORT, &hints, &res)) != 0)
	{
		std::cerr << RED << "getaddrinfo() error:" << gai_strerror(status) << RESET << std::endl;
		// gai_strerror return a text string describing an error value for the getaddrinfo() and getnameinfo()
		return -1;
	}

	for(p = res;p != NULL; p = p->ai_next)
	{
		void *addr;
		char *ipver;

		// get the pointer to the address itself,
		// different fields in IPv4 and IPv6:

		if (p->ai_family == AF_INET) // IPv4
		{
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
			addr = &(ipv4->sin_addr);
			ipver = "IPv4";
		}
		else // IPv6
		{
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
			addr = &(ipv6->sin6_addr);
			ipver = "IPv6";
		}
		inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
		printf(" %s: %s\n", ipver, ipstr);
	}

	int serverSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (serverSocket < 0)
	{
		std::cerr << RED << "Socket creation failed: " << strerror(errno) << RESET << std::endl;
		return -1;
	}
	std::cout << GREEN << "Socket created successfully: " << serverSocket << RESET << std::endl;

	int yes = 1;
	setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	int _bind = bind(serverSocket, res->ai_addr, res->ai_addrlen);
	if (_bind < 0 )
	{
		std::cerr << RED << "Bind failed: " << strerror(errno) << RESET << std::endl;
		return -1;
	}
	std::cout << GREEN << "Bind successfully: " << _bind << RESET << std::endl;

	int _listen = listen(serverSocket, BACKLOG);
	if (_listen < 0 )
	{
		std::cerr << RED << "Listen failed: " << strerror(errno) << RESET << std::endl;
		return -1;
	}
	std::cout << GREEN << "Listen successfully: " << _listen << RESET << std::endl;

	char buffer[30000] = {0};
	while (true)
	{
		addr_size = sizeof(other_addr);
		int clinetSocket = accept(serverSocket, (struct sockaddr *)&other_addr, &addr_size);
		if (clinetSocket < 0 )
		{
			std::cerr << RED << "Accept failed: " << strerror(errno) << RESET << std::endl;
			return -1;
		}
		std::cout << GREEN << "Accept successfully: " << clinetSocket << RESET << std::endl;
		
		memset(buffer, 0, sizeof(buffer));

		ssize_t	bytesReceived = recv(clinetSocket, buffer, sizeof(buffer), 0);
		if (bytesReceived < 0) {
			std::cerr << RED << "Recv failed: " << strerror(errno) << RESET << std::endl;
			close(serverSocket);
			close(clinetSocket);
		}
		else {
			std::cout << GREEN << "Received:" << bytesReceived << RESET << std::endl;
			std::cout << YELLOW << "Client Request:\n" << buffer << RESET << std::endl;
		}

		// simple HTTP response
		std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 17\r\n\r\nHello from Server";

		ssize_t	bytesSent = send(clinetSocket, response.c_str(), response.length(), 0);
		if (bytesSent < 0) {
			std::cerr << RED << "Send failed: " << strerror(errno) << RESET << std::endl;
			close(serverSocket);
			close(clinetSocket);
		}
		else {
			std::cout << GREEN << "Send:" << bytesSent << RESET << std::endl;
		}
		close(clinetSocket);
	}
	close(serverSocket);

	std::cout << "End of the program" << std::endl;
	freeaddrinfo(res);
	return 0;
}
