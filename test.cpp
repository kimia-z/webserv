#include <iostream>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define domain AF_UNSPEC
#define service SOCK_STREAM
#define protocol 0
#define port 8081
#define interface INADDR_ANY
#define backlog 10

int main()
{
	struct sockaddr_in _address;
	char _buffer[30000];

	memset(&_address, 0, sizeof(_address));
	_address.sin_addr.s_addr = htonl(interface);
	_address.sin_family = domain;
	_address.sin_port = htons(port);

	socklen_t addrlen = sizeof(_address);

	int _socket = socket(domain, service, protocol);
	if (_socket < 0)
	{
		perror("Failed to create Socket...");
		return -1;
	}

	int _bind = bind(_socket, (struct sockaddr *)&_address, sizeof(_address));
	if (_bind < 0 )
	{
		perror("Failed to Bind...");
		return -1;
	}

	int _listen = listen(_socket, backlog);
	if (_listen < 0)
	{
		perror("Failed to Listen...");
		return -1;
	}

	while(true)
	{
		std::cout << "====== WAITING ======" << std::endl;
		int _newSocket = accept(_socket, (struct sockaddr *)&_address, &addrlen);
		if (_newSocket < 0)
		{	
			perror("Failed to Accept...");
			return -1;
		}
		std::cout << "Accepted connection from " << inet_ntoa(_address.sin_addr) << ":" << ntohs(_address.sin_port) << std::endl;
		memset(_buffer, 0, sizeof(_buffer));
		int bytes_read = read(_newSocket, _buffer, sizeof(_buffer));
		if (bytes_read <= 0)
		{
			perror("Failed to read...");
			close(_newSocket);
		}
		std::cout << "Received: " << bytes_read << "bytes" << std::endl;
		if (strlen(_buffer) > 0)
		{
			std::cout << "Handeling: " << _buffer << std::endl;
			const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 17\r\n\r\nHello from Server";
			write(_newSocket, response, strlen(response));
			close(_newSocket);
			std::cout << "====== DONE ======" << std::endl;
		}
	}
}
