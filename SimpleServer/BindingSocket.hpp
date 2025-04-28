#pragma once

#include "SimpleSocket.hpp"
#include <unistd.h>

class BindingSocket : public SimpleSocket
{
public:
	BindingSocket(int domain, int service, int portocol, int port, u_long interface);
	int connectToNetwork(int socket, struct sockaddr_in address);
};



