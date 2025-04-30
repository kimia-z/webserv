#pragma once
#include "SimpleSocket.hpp"
#include "BindingSocket.hpp"
#include "ConnectingSocket.hpp"
#include "ListeningSocket.hpp"
#include "SimpleServer.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class TestServer : public SimpleServer
{
private:
	int _newSocket;
	char _buffer[30000];
	
public:
	TestServer();
	void accepter() override;
	void handler() override;
	void responder() override;

	void launch() override;
};


