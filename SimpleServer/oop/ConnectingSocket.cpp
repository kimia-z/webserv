#include "ConnectingSocket.hpp"

ConnectingSocket::ConnectingSocket(int domain, int service, int portocol, int port, u_long interface) : SimpleSocket(domain, service, portocol, port, interface)
{
	setConnection(connectToNetwork(getSocket(), getAddress()));
	testConnection(getConnection());
}


int ConnectingSocket::connectToNetwork(int socket, struct sockaddr_in address)
{
	return connect(socket, (struct sockaddr *)&address, sizeof(address));
}