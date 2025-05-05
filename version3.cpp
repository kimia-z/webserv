#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT "8081"
#define BACKLOG 10

int main() {
    struct addrinfo hints, *result, *resultCopy;
    int serverFd;
    char buffer[30000];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;      // IPv6 - will be adjusted later with setsockopt() for IPv4-mapped
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_flags = AI_PASSIVE;      // For wildcard IP

    int status = getaddrinfo(NULL, PORT, &hints, &result);
    if (status != 0) {
        std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
        return 1;
    }

    for (resultCopy = result; resultCopy != nullptr; resultCopy = resultCopy->ai_next) {

        serverFd = socket(resultCopy->ai_family, resultCopy->ai_socktype, resultCopy->ai_protocol);
		if (serverFd == -1)
			continue;
		int	no = 0;
		setsockopt(serverFd, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no));
        int yes = 1;
        setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
        if (bind(serverFd, resultCopy->ai_addr, resultCopy->ai_addrlen) == 0)
			break;
        close(serverFd);
    }

    if (resultCopy == nullptr) {
        std::cerr << "Failed to bind\n";
        return 2;
    }

    freeaddrinfo(result);

    if (listen(serverFd, BACKLOG) == -1) {
        perror("listen");
		close(serverFd);
        return 3;
    }

    std::cout << "Server running on port " << PORT << std::endl;

    while (true) {
        struct sockaddr_storage client_addr;
        socklen_t addr_size = sizeof(client_addr);
        int client_fd = accept(serverFd, (struct sockaddr*)&client_addr, &addr_size);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes > 0) {
            std::cout << "Received:\n" << buffer << std::endl;

            const char* response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Length: 17\r\n"
                "Content-Type: text/plain\r\n\r\n"
                "Hello from Server";
            send(client_fd, response, strlen(response), 0);
        }

        close(client_fd);
    }

    close(serverFd);
    return 0;
}