#include "SimpleSocket.hpp"

// Default Constraction

SimpleSocket::SimpleSocket(int domain, int service, int protocol, int port, u_long interface)
{
	// Define Address Struct
	_address.sin_addr.s_addr = htonl(interface);
	_address.sin_family = domain;
	_address.sin_port = htons(port);

	// Establish Socket
	_socket = socket(domain, service, protocol);
	testConnection(_socket);

	// Establish Network Connection
	_connection = connectToNetwork(_socket, _address);
	testConnection(_connection);
}

// Test Socket/Connection

void SimpleSocket::testConnection(int itemToTest)
{
	if (itemToTest < 0)
	{
		perror("Failed to connect...");
		exit(EXIT_FAILURE);
	}
}

// Getters

int SimpleSocket::getSocket()
{
	return _socket;
}
int SimpleSocket::getConnection()
{
	return _connection;
}
struct sockaddr_in SimpleSocket::getAddress()
{
	return _address;
}