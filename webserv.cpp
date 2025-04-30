
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[30000] = {0};
    socklen_t addr_len = sizeof(client_addr);

    // Tworzenie socketu
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Socket failed");
        return 1;
    }

    // Konfiguracja adresu serwera
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // nasłuchuj na wszystkich interfejsach
    server_addr.sin_port = htons(PORT);

    // Przypisz socket do portu
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }

    // Nasłuchuj połączeń
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }

    std::cout << "Serwer działa na porcie " << PORT << "..." << std::endl;

    // Akceptuj połączenie od klienta
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0) {
        perror("Accept failed");
        close(server_fd);
        return 1;
    }

    // read(client_fd, buffer, 30000);
    // std::cout << "Odebrano żądanie:\n" << buffer << std::endl;

    // // Prosta odpowiedź HTTP
    // std::string response =
    //     "HTTP/1.1 200 OK\r\n"
    //     "Content-Type: text/html\r\n"
    //     "Content-Length: 46\r\n"
    //     "\r\n"
    //     "<html><body><h1>Hello, world!</h1></body></html>";

    // write(client_fd, response.c_str(), response.length());

	ssize_t	bytesReceived = recv(client_fd, buffer, sizeof(buffer), 0);
	if (bytesReceived < 0) {
		perror("recv nie dziala");
	}
	else {
		std::cout << "Odebrano żądanie:\n" << buffer << std::endl;
	}

	// Prosta odpowiedź HTTP
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 46\r\n"
        "\r\n"
        "<html><body><h1>Hello, world!</h1></body></html>";

	ssize_t	bytesSent = send(client_fd, response.c_str(), response.length(), 0);
	if (bytesSent < 0) {
		perror ("send nie dziala");
	}

    close(client_fd);
    close(server_fd);

	std::cout << "Koniec programu" << std::endl;

    return 0;
}