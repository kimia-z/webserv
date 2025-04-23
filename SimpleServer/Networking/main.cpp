#include "SimpleSocket.hpp"
#include "BindingSocket.hpp"
#include "ConnectingSocket.hpp"
#include "ListeningSocket.hpp"
#include "SimpleServer.hpp"
#include "TestServer.hpp"

int main()
{
	/*
	std::cout << "Starting.." << std::endl;
	//SimpleSocket ss = SimpleSocket(AF_INET, SOCK_STREAM, 0, 80, INADDR_ANY);
	std::cout << "Binding Sockect.." << std::endl;
	BindingSocket bs = BindingSocket(AF_INET, SOCK_STREAM, 0, 8081, INADDR_ANY);
	std::cout << "Listening Sockect.." << std::endl;
	ListeningSocket ls = ListeningSocket(AF_INET, SOCK_STREAM, 0, 8080, INADDR_ANY, 10);
	std::cout << "End.." << std::endl;
	*/

	TestServer t;
}