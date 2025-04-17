#pragma once

#include "SimpleSocket.hpp"

class BindingSocket : public SimpleSocket
{
private:
	/* data */
public:
	BindingSocket(int domain, int service, int portocol, int port, u_long interface);
	int connectToNetwork(int socket, struct sockaddr_in address);
};



