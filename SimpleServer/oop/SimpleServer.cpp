#include "SimpleServer.hpp"

SimpleServer::SimpleServer(int domain, int service, int portocol,
						   int port, u_long interface, int backlog)
						   : _socket(new ListeningSocket(domain, service, portocol, port, interface, backlog))
{}

SimpleServer::~SimpleServer()
{
	delete _socket;
}

ListeningSocket *SimpleServer::getSocket()
{
	return _socket;
}