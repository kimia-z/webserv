#pragma once

#include "ListeningSocket.hpp"

class SimpleServer
{
protected:
	ListeningSocket * _socket;
	
public:
	SimpleServer(int domain, int service, int portocol, int port, u_long interface, int backlog);
	virtual ~SimpleServer();
	ListeningSocket *getSocket();
	virtual void accepter() = 0;
	virtual void handler() = 0;
	virtual void responder() = 0;
	virtual void launch() = 0;
};