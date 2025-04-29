#include "TestServer.hpp"


TestServer::TestServer() : SimpleServer(AF_INET, SOCK_STREAM, 0, 8081, INADDR_ANY, 10)
{
	launch();
}

void TestServer::accepter()
{
	struct sockaddr_in address = getSocket()->getAddress();
	socklen_t addrlen = sizeof(address);
	_newSocket = accept(getSocket()->getSocket(), (struct sockaddr *)&address, &addrlen);
	if (_newSocket < 0)
		perror("sockect failed");
	std::cout << "Accepted connection from " << inet_ntoa(address.sin_addr) << ":" << ntohs(address.sin_port) << std::endl;
	memset(_buffer, 0, sizeof(_buffer));
	int bytes_read = read(_newSocket, _buffer, sizeof(_buffer));
	if (bytes_read <= 0)
	{
		perror("errof fd");
		close(_newSocket);
	}
	// _buffer[bytes_read] = '\0';
	std::cout << "Received: " << bytes_read << "bytes" << std::endl;
}

void TestServer::handler()
{
	std::cout << "Handeling: " << _buffer << std::endl;
}

void TestServer::responder()
{
	const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 17\r\n\r\nHello from Server";
	write(_newSocket, response, strlen(response));
	close(_newSocket);
}

void TestServer::launch()
{
	while(true)
	{
		std::cout << "====== WAITING ======" << std::endl;
		accepter();
		if (strlen(_buffer) > 0)
		{
			handler();
			responder();
			std::cout << "====== DONE ======" << std::endl;
		}
	}
}