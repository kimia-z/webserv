
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8081

int main() {
    int server_fd, client_fd;
    struct sockaddr_in6 server_addr;
	struct sockaddr_in	client_addr;
    char buffer[30000] = {0};
	int	no = 0;
    socklen_t addr_len = sizeof(client_addr);

	memset(&server_addr, 0, sizeof(server_addr));

    // creating the socket
    server_fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Socket failed");
        return 1;
    }
	setsockopt(server_fd, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no));
    // configuration server's address
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any; // listen on any interface
    server_addr.sin6_port = htons(PORT);

    // Bind the socket to the port
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }

    // Listen to connections
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }

    std::cout << "Server works on the port " << PORT << "..." << std::endl;

	while (true)
	{
		memset(&client_addr, 0, sizeof(client_addr));
		// Accept client's requests
		client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
		if (client_fd < 0) {
			perror("accept failed");
			close(server_fd);
			return 1;
		}
		std::cout << "accepted connection from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(server_addr.sin6_port) << std::endl;

		memset(buffer, 0, sizeof(buffer));

		ssize_t	bytesReceived = recv(client_fd, buffer, sizeof(buffer), 0);
		if (bytesReceived < 0) {
			perror("recv failed");
			close(server_fd);
			close(client_fd);
		}
		else {
			std::cout << "Received:\n" << buffer << std::endl;
		}

		// simple HTTP response
		std::string response =
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/html\r\n"
			"Content-Length: 46\r\n"
			"\r\n"
			"<html><body><h1>Hello, world!</h1></body></html>";

		ssize_t	bytesSent = send(client_fd, response.c_str(), response.length(), 0);
		if (bytesSent < 0) {
			perror ("send failed");
			close(server_fd);
			close(client_fd);
		}
		close(client_fd);
	}
    close(server_fd);

	std::cout << "End of the program" << std::endl;

    return 0;
}
