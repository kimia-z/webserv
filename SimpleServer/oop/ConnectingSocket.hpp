#pragma once

#include "SimpleSocket.hpp"

class ConnectingSocket : public SimpleSocket
{
private:
	/* data */
public:
	ConnectingSocket(int domain, int service, int portocol, int port, u_long interface);
	int connectToNetwork(int socket, struct sockaddr_in address);
};



