#include "BindingSocket.hpp"

BindingSocket::BindingSocket(int domain, int service, int portocol, int port, u_long interface) : SimpleSocket(domain, service, portocol, port, interface)
{
	setConnection(connectToNetwork(getSocket(), getAddress()));
	testConnection(getConnection());
}


int BindingSocket::connectToNetwork(int socket, struct sockaddr_in address)
{
	return bind(socket, (struct sockaddr *)&address, sizeof(address));
}