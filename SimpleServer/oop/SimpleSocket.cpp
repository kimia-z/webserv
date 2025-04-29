#include "SimpleSocket.hpp"

// Default Constraction

SimpleSocket::SimpleSocket(int domain, int service, int protocol, int port, u_long interface)
{
	// Define Address Struct
	memset(&_address, 0, sizeof(_address));
	_address.sin_addr.s_addr = htonl(interface);
	_address.sin_family = domain;
	_address.sin_port = htons(port);

	// Establish Socket
	_socket = socket(domain, service, protocol);
	testConnection(_socket);
	_connection = -1;
}

SimpleSocket::~SimpleSocket()
{
	if (_socket >= 0)
		close(_socket);
	if (_connection >= 0)
		close(_connection);
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

int SimpleSocket::getSocket() const
{
	return _socket;
}
int SimpleSocket::getConnection() const
{
	return _connection;
}
const struct sockaddr_in& SimpleSocket::getAddress() const
{
	return _address;
}

// Setter

void SimpleSocket::setConnection(int connection)
{
	_connection = connection;
}