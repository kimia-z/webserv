#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <unistd.h>

class SimpleSocket
{
protected:
	int _socket;
	int _connection;
	struct sockaddr_in _address;
public:
	SimpleSocket(int domain, int service, int protocol, int port, u_long interface);
	virtual ~SimpleSocket();
	// Virtual function to connect to a network
	virtual int connectToNetwork(int socket, struct sockaddr_in address) = 0;

	// Funtion to test
	void testConnection(int itemToTest);
	
	// Getter functions
	int getSocket() const;
	int getConnection() const;
	const struct sockaddr_in& getAddress() const;

	// Setter functions
	void setConnection(int connection);
};



