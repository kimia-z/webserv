#include "ListeningSocket.hpp"


ListeningSocket::ListeningSocket(int domain, int service, int portocol, int port, u_long interface, int backlog) : BindingSocket(domain, service, portocol, port, interface), _backlog(backlog)
{
	// _backlog = backlog;
	startListening();
	testConnection(_listening);
}

void ListeningSocket::startListening()
{
	_listening = listen(getSocket(), _backlog);
	//close(getSocket());
}