#pragma once

#include "BindingSocket.hpp"

class ListeningSocket : public BindingSocket
{
private:
	int _backlog;
	int _listening;
public:
	ListeningSocket(int domain, int service, int portocol, int port, u_long interface, int backlog);
	void startListening();
};


