#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <iostream>

class SimpleSocket
{
private:
	int _socket;
	int _connection;
	struct sockaddr_in _address;
public:
	SimpleSocket(int domain, int service, int protocol, int port, u_long interface);
	
	// Virtual function to connect to a network
	virtual int connectToNetwork(int socket, struct sockaddr_in address) = 0;

	// Funtion to test
	void testConnection(int itemToTest);
	
	// Getter functions
	int getSocket();
	int getConnection();
	struct sockaddr_in getAddress();
};



